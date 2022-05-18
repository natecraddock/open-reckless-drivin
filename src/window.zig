//! window.zig: a small wrapper around SDL2 for controlling windowing, events,
//! pixel drawing, and audio.

const sdl = @import("sdl");
const std = @import("std");

pub const width: usize = 640;
pub const height: usize = 480;

/// Contains logic to manage the state of the Reckless Drivin' window
///
/// SDL provides more than just cross-platform window management (audio), but
/// for simplicity we will just use the Window struct for all interfacing with
/// the OS I/O.
pub const Window = struct {
    window: sdl.Window,
    renderer: sdl.Renderer,
    texture: sdl.Texture,

    /// Initialize the window and return a struct
    pub fn init() !Window {
        try sdl.init(.{
            .video = true,
            .events = true,
        });

        var window = try sdl.createWindow(
            "Reckless Drivin'",
            .{ .centered = {} },
            .{ .centered = {} },
            width,
            height,
            .{
                .hidden = true,
                .resizable = true,
                .allow_high_dpi = true,
            },
        );

        var renderer = try sdl.createRenderer(window, null, .{});
        try renderer.setLogicalSize(width, height);

        // TODO: for some reason this needs to be done after the renderer?
        window.setMinimumSize(width, height);

        // Don't interpolate pixels when scaled
        _ = sdl.c.SDL_SetHint(sdl.c.SDL_HINT_RENDER_SCALE_QUALITY, "0");

        var texture = try sdl.createTexture(
            renderer,
            .argb8888,
            .streaming,
            width,
            height,
        );

        sdl.c.SDL_ShowWindow(window.ptr);

        return Window{
            .window = window,
            .renderer = renderer,
            .texture = texture,
        };
    }

    /// Free all memory created by the window
    pub fn deinit(self: *Window) void {
        self.texture.destroy();
        self.renderer.destroy();
        self.window.destroy();
        sdl.quit();
    }

    /// Copy a buffer of pixels to the window for display
    pub fn render(self: *Window, pixels: []const u8) !void {
        // try self.texture.update(pixels, width * 4, null);
        _ = pixels;
        try self.renderer.clear();
        try self.renderer.copy(self.texture, null, null);
        self.renderer.present();
        sdl.delay(10);
    }

    pub fn getEvent() ?sdl.Event {
        return sdl.pollEvent();
    }
};
