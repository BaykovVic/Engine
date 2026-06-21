using System;
using Avalonia.Controls;
using Avalonia.Platform;
using Avalonia.Threading;
using Avalonia.VisualTree;
using SkyEditor.Engine;

namespace SkyEditor.Controls;

/// Embeds the engine's Vulkan swapchain into the editor as a child native
/// window — the UI toolkit owns the window and the renderer draws into it,
/// the architecture we committed to. The native session handle is shared with
/// the panels so the viewport shows the exact scene they edit.
public sealed class VulkanViewport : NativeControlHost
{
    private IntPtr _context;
    private ulong _window;
    private DispatcherTimer? _timer;
    private bool _attached;
    private uint _width;
    private uint _height;

    public void SetContext(IntPtr context) => _context = context;

    protected override IPlatformHandle CreateNativeControlCore(IPlatformHandle parent)
    {
        var handle = base.CreateNativeControlCore(parent);
        if (handle.HandleDescriptor == "XID")
            _window = (ulong)handle.Handle.ToInt64();

        // Attach lazily on the first tick, once the control has a real size.
        _timer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(16) };
        _timer.Tick += OnTick;
        _timer.Start();
        return handle;
    }

    private void OnTick(object? sender, EventArgs e)
    {
        if (_context == IntPtr.Zero || _window == 0)
            return;

        if (!_attached)
        {
            if (Bounds.Width < 16 || Bounds.Height < 16)
                return; // wait for layout
            var scale = (this.GetVisualRoot()?.RenderScaling) ?? 1.0;
            _width = (uint)(Bounds.Width * scale);
            _height = (uint)(Bounds.Height * scale);
            var ok = EngineInterop.sky_editor_attach_viewport(
                _context, IntPtr.Zero, _window, _width, _height);
            Console.Error.WriteLine($"[viewport] attach {_width}x{_height} -> {ok}");
            _attached = ok == 1;
            if (!_attached)
            {
                _timer?.Stop();
                return;
            }
        }

        EngineInterop.sky_editor_render_viewport(_context, _width, _height);
    }

    protected override void DestroyNativeControlCore(IPlatformHandle control)
    {
        _timer?.Stop();
        _timer = null;
        if (_attached && _context != IntPtr.Zero)
            EngineInterop.sky_editor_detach_viewport(_context);
        _attached = false;
        base.DestroyNativeControlCore(control);
    }
}
