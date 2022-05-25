//! window.zig: a small wrapper around SDL2 for controlling windowing, events,
//! pixel drawing, and audio.

const sdl = @import("sdl");
const std = @import("std");

const Allocator = std.mem.Allocator;

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

    pixels: []u16,

    /// Initialize the window and return a struct
    pub fn init(allocator: Allocator) !Window {
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

        // TODO: maybe only do rgb because this game doesn't use alpha
        var texture = try sdl.createTexture(
            renderer,
            .rgb555,
            .streaming,
            width,
            height,
        );

        var pixels = try allocator.alloc(u16, width * height);

        // Ensure a clean slate before showing the window
        try renderer.clear();
        renderer.present();

        sdl.c.SDL_ShowWindow(window.ptr);

        return Window{
            .window = window,
            .renderer = renderer,
            .texture = texture,
            .pixels = pixels,
        };
    }

    /// Free all memory created by the window
    pub fn deinit(self: *Window, allocator: Allocator) void {
        allocator.free(self.pixels);
        self.texture.destroy();
        self.renderer.destroy();
        self.window.destroy();
        sdl.quit();
    }

    /// Copy a buffer of pixels to the window for display
    pub fn render(self: *Window) !void {
        const pixels = @ptrCast([*]const u8, self.pixels)[0 .. width * 2];
        try self.texture.update(pixels, width * 2, null);
        try self.renderer.clear();
        try self.renderer.copy(self.texture, null, null);
        self.renderer.present();
        sdl.delay(10);
    }

    pub fn getEvent() ?sdl.Event {
        return sdl.pollEvent();
    }
};
