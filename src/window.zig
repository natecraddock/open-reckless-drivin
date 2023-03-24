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

        // Reckless drivin stores pixels in RGB555 (0RRRRRGG_GGGBBBBB), while Raylib does
        // RGBA555 (RRRRRGGG_GGBBBBBA). So we need to convert the pixels. This seems to be fast
        // enough for now, but maybe someday I'll update the logic throughout reckless drivin to
        // set the pixels correctly up front.
        for (win.pixels, 0..) |pix, i| {
            win.tex_pixels[i] = pix << 1;
            win.tex_pixels[i] |= 1;
        }
        raylib.c.UpdateTexture(win.texture, @ptrCast(*anyopaque, win.tex_pixels));

        const win_width = @intToFloat(f32, raylib.c.GetRenderWidth());
        const win_height = @intToFloat(f32, raylib.c.GetRenderHeight());

        const scale = @min(win_width / width, win_height / height);
        const output_width = width * scale;
        const output_height = height * scale;

        const dest = raylib.c.Rectangle{
            .x = (win_width - output_width) / 2.0,
            .y = (win_height - output_height) / 2.0,
            .width = output_width,
            .height = output_height,
        };

        raylib.c.DrawTexturePro(
            win.texture,
            .{ .x = 0, .y = 0, .width = width, .height = height },
            dest,
            .{ .x = 0, .y = 0 },
            0,
            raylib.c.WHITE,
        );

        raylib.endDrawing();
    }

    pub fn getEvent() ?void {
        // return sdl.pollEvent();
        return null;
    }

    pub fn shouldClose() bool {
        return raylib.c.WindowShouldClose();
    }
};
