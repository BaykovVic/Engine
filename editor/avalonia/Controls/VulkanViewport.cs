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

        for (var i = 0; i < 3; ++i)
        {
            var (ax, ay, az) = AxisDirs[i];
            worldAxes[i] = LocalSpace ? RotateByQuat(rotation, ax, ay, az) : (ax, ay, az);
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

    private void DrawGizmo(DrawingContext context)
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

    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        base.OnPointerPressed(e);
        if (GameView)
            return; // the Game view has no editor camera/gizmo interaction
        var position = e.GetPosition(this);
        var point = e.GetCurrentPoint(this).Properties;

        if (point.IsLeftButtonPressed && _context != IntPtr.Zero &&
            GizmoAxes(out var origin, out var tips, out var worldPos, out var worldAxes))
        {
            var best = -1;
            var bestDistance = 9.0; // pixels
            for (var i = 0; i < 3; ++i)
            {
                var distance = DistanceToSegment(position, origin, tips[i]);
                if (distance < bestDistance)
                {
                    bestDistance = distance;
                    best = i;
                }
            }
            if (best >= 0)
            {
                _dragAxis = best;
                _dragStartPointer = position;
                _dragAxisScreen = tips[best] - origin;
                _dragAxisWorld = worldAxes[best];
                _dragStartWorld[0] = worldPos[0];
                _dragStartWorld[1] = worldPos[1];
                _dragStartWorld[2] = worldPos[2];
                e.Pointer.Capture(this);
                e.Handled = true;
                return;
            }
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
        if (_dragAxis >= 0)
        {
            _dragAxis = -1;
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
