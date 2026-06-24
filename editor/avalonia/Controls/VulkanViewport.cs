using System;
using System.Globalization;
using System.Runtime.InteropServices;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Platform;
using Avalonia.Threading;
using Avalonia.VisualTree;
using SkyEditor.Engine;

namespace SkyEditor.Controls;

/// The editor's 3D viewport. The engine renders the scene offscreen through
/// the orbit camera; the control blits the result into a bitmap each frame,
/// draws the move gizmo over the selection (in world or the object's own local
/// space) and turns pointer input into camera orbit/pan/zoom, click-to-pick
/// and gizmo axis drags. An ordinary Avalonia control — input and overlays
/// work, so the renderer embeds into the UI, not the other way round.
public sealed class VulkanViewport : Control
{
    private const float GizmoLength = 1.2f; // world units along each axis

    private IntPtr _context;
    private WriteableBitmap? _bitmap;
    private byte[]? _buffer;
    private PixelSize _size;
    private DispatcherTimer? _timer;

    private Point _lastPointer;
    private bool _orbiting;
    private bool _panning;
    private bool _moved;
    private bool _rendered;     // a frame was produced at least once
    private int _failedFrames;  // consecutive render_offscreen failures

    private int _dragAxis = -1;
    private Point _dragStartPointer;
    private Vector _dragAxisScreen;
    private (float X, float Y, float Z) _dragAxisWorld;
    private readonly float[] _dragStartWorld = new float[3];
    private readonly float[] _scaleStart = new float[3]; // local scale at drag start
    private bool _scaleUniform;

    // Rotate gizmo drag state.
    private int _rotateAxis = -1;
    private (float X, float Y, float Z) _rotateAxisWorld;
    private Point _rotateOrigin;     // projected object origin (screen)
    private double _rotateLastAngle; // last pointer angle around the origin
    private double _rotateSign;      // screen-to-rotation sign for this axis

    private const int RingSegments = 48;

    private static readonly Color[] AxisColors =
    {
        Color.Parse("#F0626E"), Color.Parse("#62C76E"), Color.Parse("#5A93F8"),
    };
    private static readonly (float X, float Y, float Z)[] AxisDirs =
    {
        (1f, 0f, 0f), (0f, 1f, 0f), (0f, 0f, 1f),
    };

    /// The selected object the gizmo is drawn for (0 = none).
    public ulong SelectedId { get; set; }

    /// When true, the gizmo axes follow the object's own orientation (local
    /// space); otherwise they are world-aligned (global space).
    public bool LocalSpace { get; set; }

    /// When true, render through the scene's Main Camera (the Game view) with
    /// no gizmo or editor camera input.
    public bool GameView { get; set; }

    /// Active manipulation tool: Hand (camera only), Move, Rotate or Scale.
    public GizmoTool Tool { get; set; } = GizmoTool.Move;

    /// Raised with the picked object's native id (0 = empty space).
    public event Action<ulong>? ObjectPicked;

    public VulkanViewport()
    {
        // Keep the rendered scene and the gizmo overlay inside the viewport —
        // otherwise a gizmo for an object projected off-screen overdraws the
        // toolbar and window chrome.
        ClipToBounds = true;
    }

    public void SetContext(IntPtr context) => _context = context;

    /// Renders a single frame synchronously (used for headless capture).
    public void RenderOnce() => RenderFrame();

    protected override void OnAttachedToVisualTree(VisualTreeAttachmentEventArgs e)
    {
        base.OnAttachedToVisualTree(e);
        _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(16) };
        _timer.Tick += (_, _) => RenderFrame();
        _timer.Start();
    }

    protected override void OnDetachedFromVisualTree(VisualTreeAttachmentEventArgs e)
    {
        _timer?.Stop();
        _timer = null;
        base.OnDetachedFromVisualTree(e);
    }

    private void RenderFrame()
    {
        if (_context == IntPtr.Zero)
            return;

        var scale = (this.GetVisualRoot()?.RenderScaling) ?? 1.0;
        var width = (int)(Bounds.Width * scale);
        var height = (int)(Bounds.Height * scale);
        if (width < 8 || height < 8)
            return;

        if (_bitmap == null || _size.Width != width || _size.Height != height)
        {
            _size = new PixelSize(width, height);
            _bitmap = new WriteableBitmap(_size, new Vector(96, 96),
                PixelFormat.Rgba8888, AlphaFormat.Opaque);
            _buffer = new byte[width * height * 4];
        }

        var rendered = GameView
            ? EngineInterop.sky_editor_render_game_offscreen(_context, (uint)width, (uint)height, _buffer!, _buffer!.Length)
            : EngineInterop.sky_editor_render_offscreen(_context, (uint)width, (uint)height, _buffer!, _buffer!.Length);
        if (rendered != 1)
        {
            // The offscreen Vulkan renderer produced nothing (e.g. no Vulkan
            // device). Surface that instead of a silent black viewport.
            if (!_rendered && ++_failedFrames >= 3)
                InvalidateVisual();
            return;
        }
        _rendered = true;
        _failedFrames = 0;

        using (var locked = _bitmap.Lock())
        {
            var rowBytes = width * 4;
            if (locked.RowBytes == rowBytes)
            {
                Marshal.Copy(_buffer!, 0, locked.Address, _buffer!.Length);
            }
            else
            {
                for (var y = 0; y < height; ++y)
                    Marshal.Copy(_buffer!, y * rowBytes,
                        locked.Address + y * locked.RowBytes, rowBytes);
            }
        }
        InvalidateVisual();
    }

    public override void Render(DrawingContext context)
    {
        if (_rendered && _bitmap != null)
        {
            context.DrawImage(_bitmap, new Rect(Bounds.Size));
        }
        else if (_failedFrames >= 3)
        {
            var text = new FormattedText(
                "3D viewport unavailable.\n\nThe Vulkan renderer could not be created.\n" +
                "Install a Vulkan driver (MoltenVK via the Vulkan SDK on macOS) and rebuild.",
                CultureInfo.CurrentCulture, FlowDirection.LeftToRight, Typeface.Default,
                14, new SolidColorBrush(Color.Parse("#9AA1AC")));
            context.DrawText(text, new Point(24, Math.Max(24, Bounds.Height / 2 - 40)));
        }
        if (!GameView)
            DrawGizmo(context);
    }

    private void DrawGizmo(DrawingContext context)
    {
        switch (Tool)
        {
            case GizmoTool.Move: DrawMoveGizmo(context); break;
            case GizmoTool.Rotate: DrawRotateGizmo(context); break;
            case GizmoTool.Scale: DrawScaleGizmo(context); break;
            // Hand: camera only, no gizmo.
        }
    }

    // --- Move gizmo ---------------------------------------------------------

    private static (float, float, float) RotateByQuat(float[] q, float vx, float vy, float vz)
    {
        float ux = q[0], uy = q[1], uz = q[2], w = q[3];
        float tx = 2f * (uy * vz - uz * vy);
        float ty = 2f * (uz * vx - ux * vz);
        float tz = 2f * (ux * vy - uy * vx);
        float cx = uy * tz - uz * ty;
        float cy = uz * tx - ux * tz;
        float cz = ux * ty - uy * tx;
        return (vx + w * tx + cx, vy + w * ty + cy, vz + w * tz + cz);
    }

    /// Projects the selection's origin and axis tips to control (DIP) coords,
    /// also returning the world origin and the (world or local) axis vectors
    /// the drag uses. Returns false when there is no selection on screen.
    private bool GizmoAxes(out Point origin, out Point[] tips, out float[] worldPos,
        out (float X, float Y, float Z)[] worldAxes)
    {
        origin = default;
        tips = new[] { default(Point), default(Point), default(Point) };
        worldPos = new float[3];
        worldAxes = new (float, float, float)[3];
        if (SelectedId == 0 || _context == IntPtr.Zero ||
            Bounds.Width < 1 || Bounds.Height < 1)
            return false;

        var rotation = new float[4];
        var scale = new float[3];
        EngineInterop.sky_editor_get_world_transform(_context, SelectedId, worldPos, rotation, scale);

        var w = (uint)Bounds.Width;
        var h = (uint)Bounds.Height;
        if (EngineInterop.sky_editor_project(_context, worldPos[0], worldPos[1], worldPos[2],
                w, h, out var ox, out var oy) != 1)
            return false;
        origin = new Point(ox, oy);

        // Scale acts on local axes (like Unity's scale tool); move and rotate
        // follow the Global/Local toggle.
        var useLocal = LocalSpace || Tool == GizmoTool.Scale;
        for (var i = 0; i < 3; ++i)
        {
            var (ax, ay, az) = AxisDirs[i];
            worldAxes[i] = useLocal ? RotateByQuat(rotation, ax, ay, az) : (ax, ay, az);
            if (EngineInterop.sky_editor_project(_context,
                    worldPos[0] + worldAxes[i].X * GizmoLength,
                    worldPos[1] + worldAxes[i].Y * GizmoLength,
                    worldPos[2] + worldAxes[i].Z * GizmoLength,
                    w, h, out var ex, out var ey) == 1)
                tips[i] = new Point(ex, ey);
            else
                tips[i] = origin;
        }
        return true;
    }

    private void DrawMoveGizmo(DrawingContext context)
    {
        if (!GizmoAxes(out var origin, out var tips, out _, out _))
            return;
        for (var i = 0; i < 3; ++i)
        {
            var brush = new SolidColorBrush(AxisColors[i]);
            context.DrawLine(new Pen(brush, 2.5), origin, tips[i]);
            context.DrawEllipse(brush, null, tips[i], 4.5, 4.5);
        }
    }

    // --- Scale gizmo: axis stubs with square handles + a centre box ---------

    private void DrawScaleGizmo(DrawingContext context)
    {
        if (!GizmoAxes(out var origin, out var tips, out _, out _))
            return;
        for (var i = 0; i < 3; ++i)
        {
            var brush = new SolidColorBrush(AxisColors[i]);
            context.DrawLine(new Pen(brush, 2.5), origin, tips[i]);
            var box = new Rect(tips[i].X - 4, tips[i].Y - 4, 8, 8);
            context.FillRectangle(brush, box);
        }
        // Uniform-scale handle at the centre.
        context.FillRectangle(new SolidColorBrush(Color.Parse("#E8EAED")),
            new Rect(origin.X - 4, origin.Y - 4, 8, 8));
    }

    // --- Rotate gizmo: three projected rings --------------------------------

    /// Screen-space points of the ring perpendicular to world axis `axis`,
    /// sampled around the selection origin. Empty when off-screen.
    private Point[] RingPoints(float[] worldPos, (float X, float Y, float Z) axis)
    {
        // Two unit vectors spanning the plane perpendicular to the axis.
        var (ax, ay, az) = axis;
        var refv = Math.Abs(ay) > 0.9f ? (1f, 0f, 0f) : (0f, 1f, 0f);
        var u = Normalize(Cross((ax, ay, az), refv));
        var v = Cross((ax, ay, az), u);
        var pts = new Point[RingSegments + 1];
        var n = 0;
        var w = (uint)Bounds.Width;
        var h = (uint)Bounds.Height;
        for (var s = 0; s <= RingSegments; ++s)
        {
            var t = s / (double)RingSegments * Math.PI * 2;
            float cs = (float)Math.Cos(t) * GizmoLength, sn = (float)Math.Sin(t) * GizmoLength;
            var px = worldPos[0] + u.Item1 * cs + v.Item1 * sn;
            var py = worldPos[1] + u.Item2 * cs + v.Item2 * sn;
            var pz = worldPos[2] + u.Item3 * cs + v.Item3 * sn;
            if (EngineInterop.sky_editor_project(_context, px, py, pz, w, h, out var ex, out var ey) == 1)
                pts[n++] = new Point(ex, ey);
        }
        return n == pts.Length ? pts : pts[..n];
    }

    private void DrawRotateGizmo(DrawingContext context)
    {
        if (!GizmoAxes(out _, out _, out var worldPos, out var worldAxes))
            return;
        for (var i = 0; i < 3; ++i)
        {
            var pts = RingPoints(worldPos, worldAxes[i]);
            if (pts.Length < 2)
                continue;
            var pen = new Pen(new SolidColorBrush(AxisColors[i]), 2.0);
            for (var s = 0; s < pts.Length - 1; ++s)
                context.DrawLine(pen, pts[s], pts[s + 1]);
        }
    }

    private static (float, float, float) Cross((float X, float Y, float Z) a, (float X, float Y, float Z) b) =>
        (a.Y * b.Z - a.Z * b.Y, a.Z * b.X - a.X * b.Z, a.X * b.Y - a.Y * b.X);

    private static (float, float, float) Normalize((float X, float Y, float Z) a)
    {
        var len = (float)Math.Sqrt(a.X * a.X + a.Y * a.Y + a.Z * a.Z);
        return len < 1e-6f ? a : (a.X / len, a.Y / len, a.Z / len);
    }

    private static double DistanceToSegment(Point p, Point a, Point b)
    {
        double abx = b.X - a.X, aby = b.Y - a.Y;
        double apx = p.X - a.X, apy = p.Y - a.Y;
        double lengthSq = abx * abx + aby * aby;
        if (lengthSq < 1e-3)
            return Math.Sqrt(apx * apx + apy * apy);
        double t = Math.Clamp((apx * abx + apy * aby) / lengthSq, 0, 1);
        double dx = apx - abx * t, dy = apy - aby * t;
        return Math.Sqrt(dx * dx + dy * dy);
    }

    // --- Input --------------------------------------------------------------

    /// Begins a Move/Rotate/Scale gizmo drag if the pointer grabbed a handle.
    private bool BeginGizmoDrag(Point position)
    {
        if (SelectedId == 0 ||
            !GizmoAxes(out var origin, out var tips, out var worldPos, out var worldAxes))
            return false;

        if (Tool == GizmoTool.Rotate)
        {
            var bestAxis = -1;
            var bestDist = 8.0;
            for (var i = 0; i < 3; ++i)
            {
                var pts = RingPoints(worldPos, worldAxes[i]);
                for (var s = 0; s < pts.Length - 1; ++s)
                {
                    var d = DistanceToSegment(position, pts[s], pts[s + 1]);
                    if (d < bestDist) { bestDist = d; bestAxis = i; }
                }
            }
            if (bestAxis < 0)
                return false;
            _rotateAxis = bestAxis;
            _rotateAxisWorld = worldAxes[bestAxis];
            _rotateOrigin = origin;
            _rotateLastAngle = Math.Atan2(position.Y - origin.Y, position.X - origin.X);
            // Screen Y is down (clockwise-positive). A positive screen angle
            // reads as CCW about an axis facing the camera, so flip the sign
            // when the axis points away from the viewer.
            var cam = new float[3];
            EngineInterop.sky_editor_camera_position(_context, cam);
            var dot = _rotateAxisWorld.X * (worldPos[0] - cam[0]) +
                      _rotateAxisWorld.Y * (worldPos[1] - cam[1]) +
                      _rotateAxisWorld.Z * (worldPos[2] - cam[2]);
            _rotateSign = dot >= 0 ? 1.0 : -1.0;
            return true;
        }

        // Move and Scale pick the nearest axis stub.
        var best = -1;
        var bestDistance = 9.0; // pixels
        for (var i = 0; i < 3; ++i)
        {
            var distance = DistanceToSegment(position, origin, tips[i]);
            if (distance < bestDistance) { bestDistance = distance; best = i; }
        }

        if (Tool == GizmoTool.Scale)
        {
            var dCentre = Math.Sqrt(Math.Pow(position.X - origin.X, 2) +
                                    Math.Pow(position.Y - origin.Y, 2));
            _scaleUniform = dCentre < 7.0;
            if (!_scaleUniform && best < 0)
                return false;
            var pos = new float[3];
            var rot = new float[4];
            EngineInterop.sky_editor_get_transform(_context, SelectedId, pos, rot, _scaleStart);
            _dragAxis = _scaleUniform ? 0 : best;
            _dragStartPointer = position;
            _dragAxisScreen = tips[Math.Max(best, 0)] - origin;
            return true;
        }

        // Move
        if (best < 0)
            return false;
        _dragAxis = best;
        _dragStartPointer = position;
        _dragAxisScreen = tips[best] - origin;
        _dragAxisWorld = worldAxes[best];
        _dragStartWorld[0] = worldPos[0];
        _dragStartWorld[1] = worldPos[1];
        _dragStartWorld[2] = worldPos[2];
        return true;
    }

    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        base.OnPointerPressed(e);
        if (GameView)
            return; // the Game view has no editor camera/gizmo interaction
        var position = e.GetPosition(this);
        var point = e.GetCurrentPoint(this).Properties;

        if (point.IsLeftButtonPressed && _context != IntPtr.Zero &&
            Tool != GizmoTool.Hand && BeginGizmoDrag(position))
        {
            e.Pointer.Capture(this);
            e.Handled = true;
            return;
        }

        _lastPointer = position;
        _moved = false;
        _orbiting = point.IsLeftButtonPressed;
        _panning = point.IsRightButtonPressed || point.IsMiddleButtonPressed;
        e.Pointer.Capture(this);
    }

    protected override void OnPointerMoved(PointerEventArgs e)
    {
        base.OnPointerMoved(e);
        var position = e.GetPosition(this);

        if (_rotateAxis >= 0 && _context != IntPtr.Zero)
        {
            var angle = Math.Atan2(position.Y - _rotateOrigin.Y, position.X - _rotateOrigin.X);
            var delta = angle - _rotateLastAngle;
            if (delta > Math.PI) delta -= 2 * Math.PI;
            else if (delta < -Math.PI) delta += 2 * Math.PI;
            _rotateLastAngle = angle;
            EngineInterop.sky_editor_rotate_world_axis(_context, SelectedId,
                _rotateAxisWorld.X, _rotateAxisWorld.Y, _rotateAxisWorld.Z,
                (float)(delta * _rotateSign));
            return;
        }

        if (_dragAxis >= 0 && _context != IntPtr.Zero && Tool == GizmoTool.Scale)
        {
            double f;
            if (_scaleUniform)
            {
                f = Math.Max(0.01, 1.0 + (_dragStartPointer.Y - position.Y) / 120.0);
                EngineInterop.sky_editor_set_scale(_context, SelectedId,
                    (float)(_scaleStart[0] * f), (float)(_scaleStart[1] * f),
                    (float)(_scaleStart[2] * f));
            }
            else
            {
                double dx = position.X - _dragStartPointer.X;
                double dy = position.Y - _dragStartPointer.Y;
                double lengthSq = _dragAxisScreen.X * _dragAxisScreen.X +
                                  _dragAxisScreen.Y * _dragAxisScreen.Y;
                var t = lengthSq > 1e-3
                    ? (dx * _dragAxisScreen.X + dy * _dragAxisScreen.Y) / lengthSq
                    : 0.0;
                f = Math.Max(0.01, 1.0 + t);
                var ns = (float[])_scaleStart.Clone();
                ns[_dragAxis] = (float)(_scaleStart[_dragAxis] * f);
                EngineInterop.sky_editor_set_scale(_context, SelectedId, ns[0], ns[1], ns[2]);
            }
            return;
        }

        if (_dragAxis >= 0 && _context != IntPtr.Zero)
        {
            double dx = position.X - _dragStartPointer.X;
            double dy = position.Y - _dragStartPointer.Y;
            double lengthSq = _dragAxisScreen.X * _dragAxisScreen.X +
                              _dragAxisScreen.Y * _dragAxisScreen.Y;
            if (lengthSq > 1e-3)
            {
                var t = (dx * _dragAxisScreen.X + dy * _dragAxisScreen.Y) /
                        lengthSq * GizmoLength;
                EngineInterop.sky_editor_set_world_position(_context, SelectedId,
                    _dragStartWorld[0] + _dragAxisWorld.X * (float)t,
                    _dragStartWorld[1] + _dragAxisWorld.Y * (float)t,
                    _dragStartWorld[2] + _dragAxisWorld.Z * (float)t);
            }
            return;
        }

        if (!_orbiting && !_panning)
            return;
        var dragX = (float)(position.X - _lastPointer.X);
        var dragY = (float)(position.Y - _lastPointer.Y);
        if (Math.Abs(dragX) + Math.Abs(dragY) > 2)
            _moved = true;
        if (_orbiting)
            EngineInterop.sky_editor_viewport_orbit(_context, -dragX * 0.3f, -dragY * 0.3f);
        else if (_panning)
            EngineInterop.sky_editor_viewport_pan(_context, dragX, dragY);
        _lastPointer = position;
    }

    protected override void OnPointerReleased(PointerReleasedEventArgs e)
    {
        base.OnPointerReleased(e);
        if (_dragAxis >= 0 || _rotateAxis >= 0)
        {
            _dragAxis = -1;
            _rotateAxis = -1;
            e.Pointer.Capture(null);
            return;
        }
        if (_orbiting && !_moved && _context != IntPtr.Zero)
        {
            var scale = (this.GetVisualRoot()?.RenderScaling) ?? 1.0;
            var position = e.GetPosition(this);
            var id = EngineInterop.sky_editor_pick(_context,
                (float)(position.X * scale), (float)(position.Y * scale),
                (uint)_size.Width, (uint)_size.Height);
            ObjectPicked?.Invoke(id);
        }
        _orbiting = false;
        _panning = false;
        e.Pointer.Capture(null);
    }

    protected override void OnPointerWheelChanged(PointerWheelEventArgs e)
    {
        base.OnPointerWheelChanged(e);
        if (_context != IntPtr.Zero)
            EngineInterop.sky_editor_viewport_zoom(_context, e.Delta.Y > 0 ? 0.9f : 1.1f);
    }
}
