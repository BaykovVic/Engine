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
/// the orbit camera; the control blits the result into a bitmap each frame and
/// turns pointer input into camera orbit/pan/zoom and click-to-pick. Because
/// it is an ordinary Avalonia control, it receives input and can host overlays
/// — the renderer embeds into the UI, not the other way round.
public sealed class VulkanViewport : Control
{
    private IntPtr _context;
    private WriteableBitmap? _bitmap;
    private byte[]? _buffer;
    private PixelSize _size;
    private DispatcherTimer? _timer;
    private Point _lastPointer;
    private bool _orbiting;
    private bool _panning;
    private bool _moved;

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
    }

    protected override void OnPointerPressed(PointerPressedEventArgs e)
    {
        base.OnPointerPressed(e);
        _lastPointer = e.GetPosition(this);
        _moved = false;
        var point = e.GetCurrentPoint(this).Properties;
        _orbiting = point.IsLeftButtonPressed;
        _panning = point.IsRightButtonPressed || point.IsMiddleButtonPressed;
        e.Pointer.Capture(this);
    }

    protected override void OnPointerMoved(PointerEventArgs e)
    {
        base.OnPointerMoved(e);
        if (_context == IntPtr.Zero || (!_orbiting && !_panning))
            return;
        var position = e.GetPosition(this);
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
