//! window.zig: windowing, events, pixel drawing, and audio

const raylib = @import("raylib.zig");
const std = @import("std");

const Allocator = std.mem.Allocator;

pub const width = 640;
pub const height = 480;
pub const row_bytes = width * @sizeOf(u16);

/// Contains logic to manage the state of the Reckless Drivin' window
pub const Window = struct {
    pixels: []u16,
    tex_pixels: []u16,
    texture: raylib.c.Texture,

    /// Initialize the window and return a struct
    pub fn init(allocator: Allocator) !Window {
        raylib.initWindow(width, height, "Reckless Drivin'");

        var image: raylib.c.Image = .{
            .data = undefined,
            .width = width,
            .height = height,
            .mipmaps = 1,
            .format = raylib.c.PIXELFORMAT_UNCOMPRESSED_R5G5B5A1,
        };

        return Window{
            .pixels = try allocator.alloc(u16, width * height),
            .tex_pixels = try allocator.alloc(u16, width * height),
            .texture = raylib.c.LoadTextureFromImage(image),
        };
    }

    /// Free all memory created by the window
    pub fn deinit(win: *Window, allocator: Allocator) void {
        allocator.free(win.pixels);
        allocator.free(win.tex_pixels);
        raylib.closeWindow();
    }

    /// Copy a buffer of pixels to the window for display
    pub fn render(win: *Window) !void {
        raylib.beginDrawing();
        raylib.clearBackground();

        // Copy reckless drivin pixels to the raylib texture pixel buffer
        for (win.pixels, 0..) |pix, i| {
            win.tex_pixels[i] = pix << 1;
            win.tex_pixels[i] |= 1;
        }

        raylib.c.UpdateTexture(win.texture, @ptrCast([*]u8, win.tex_pixels));
        raylib.c.DrawTexture(win.texture, 0, 0, raylib.c.WHITE);

        raylib.endDrawing();
    }

    pub fn getEvent() ?void {
        // return sdl.pollEvent();
        return null;
    }
};
