using System;
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
/// draws the move gizmo over the selection, and turns pointer input into
/// camera orbit/pan/zoom, click-to-pick and gizmo axis drags. Because it is an
/// ordinary Avalonia control, it receives input and hosts overlays — the
/// renderer embeds into the UI, not the other way round.
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

    private int _dragAxis = -1;
    private Point _dragStartPointer;
    private Vector _dragAxisScreen;
    private readonly float[] _dragStartPosition = new float[3];

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

    /// Raised with the picked object's native id (0 = empty space).
    public event Action<ulong>? ObjectPicked;

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

        if (EngineInterop.sky_editor_render_offscreen(
                _context, (uint)width, (uint)height, _buffer!, _buffer!.Length) != 1)
            return;

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
        if (_bitmap != null)
            context.DrawImage(_bitmap, new Rect(Bounds.Size));
        DrawGizmo(context);
    }

    // --- Move gizmo ---------------------------------------------------------

    /// Projects the selection's origin and its three axis tips to control
    /// (DIP) coordinates. Returns false when there is no selection on screen.
    private bool GizmoAxes(out Point origin, out Point[] tips)
    {
        origin = default;
        tips = new[] { default(Point), default(Point), default(Point) };
        if (SelectedId == 0 || _context == IntPtr.Zero ||
            Bounds.Width < 1 || Bounds.Height < 1)
            return false;

        var world = new float[3];
        EngineInterop.sky_editor_world_position(_context, SelectedId, world);
        var w = (uint)Bounds.Width;
        var h = (uint)Bounds.Height;
        if (EngineInterop.sky_editor_project(_context, world[0], world[1], world[2],
                w, h, out var ox, out var oy) != 1)
            return false;
        origin = new Point(ox, oy);

        for (var i = 0; i < 3; ++i)
        {
            if (EngineInterop.sky_editor_project(_context,
                    world[0] + AxisDirs[i].X * GizmoLength,
                    world[1] + AxisDirs[i].Y * GizmoLength,
                    world[2] + AxisDirs[i].Z * GizmoLength,
                    w, h, out var ex, out var ey) == 1)
                tips[i] = new Point(ex, ey);
            else
                tips[i] = origin;
        }
        return true;
    }

    private void DrawGizmo(DrawingContext context)
    {
        if (!GizmoAxes(out var origin, out var tips))
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

    /// The gizmo axis under the pointer (-1 = none).
    private int AxisAt(Point position)
    {
        if (!GizmoAxes(out var origin, out var tips))
            return -1;
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
        return best;
    }

    // --- Input --------------------------------------------------------------

    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        base.OnPointerPressed(e);
        var position = e.GetPosition(this);
        var point = e.GetCurrentPoint(this).Properties;

        if (point.IsLeftButtonPressed && _context != IntPtr.Zero)
        {
            var axis = AxisAt(position);
            if (axis >= 0 && GizmoAxes(out var origin, out var tips))
            {
                _dragAxis = axis;
                _dragStartPointer = position;
                _dragAxisScreen = tips[axis] - origin;
                EngineInterop.sky_editor_world_position(_context, SelectedId, _dragStartPosition);
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
            var delta = position - _dragStartPointer;
            var lengthSq = _dragAxisScreen.X * _dragAxisScreen.X +
                           _dragAxisScreen.Y * _dragAxisScreen.Y;
            if (lengthSq > 1e-3)
            {
                var t = (delta.X * _dragAxisScreen.X + delta.Y * _dragAxisScreen.Y) /
                        lengthSq * GizmoLength;
                EngineInterop.sky_editor_set_position(_context, SelectedId,
                    _dragStartPosition[0] + AxisDirs[_dragAxis].X * (float)t,
                    _dragStartPosition[1] + AxisDirs[_dragAxis].Y * (float)t,
                    _dragStartPosition[2] + AxisDirs[_dragAxis].Z * (float)t);
            }
            return;
        }

        if (!_orbiting && !_panning)
            return;
        var dx = (float)(position.X - _lastPointer.X);
        var dy = (float)(position.Y - _lastPointer.Y);
        if (Math.Abs(dx) + Math.Abs(dy) > 2)
            _moved = true;
        if (_orbiting)
            EngineInterop.sky_editor_viewport_orbit(_context, -dx * 0.3f, -dy * 0.3f);
        else if (_panning)
            EngineInterop.sky_editor_viewport_pan(_context, dx, dy);
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
