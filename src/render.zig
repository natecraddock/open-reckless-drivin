//! render.zig: contains all code for drawing pixels to the screen!

const packs = @import("packs.zig");
const std = @import("std");
const window = @import("window.zig");

const Game = @import("game.zig").Game;
const Level = @import("levels.zig").Level;

pub const fps: f32 = 60.0;
pub const frame_duration = 1.0 / fps;
const zoom_velocity_factor = 50.0;
const x_camera_screen_pos = 0.5;
const y_camera_screen_pos = 0.55;

/// Render a single frame of the game
pub fn renderFrame(game: *Game) !void {
    const zoom = 0.5 + (game.zoom_vel / zoom_velocity_factor) + (game.player.obj.data.jump_height * 0.05);

    // The original source has a gCameraObj to keep track of the camera. But it
    // is only ever set to gPlayerObj, so we should be safe to just assume the
    // camera will always track the player in this implementation.
    const camera = game.player.obj.data;
    const x_draw_start = camera.pos.x - @intToFloat(f32, window.width) * x_camera_screen_pos * zoom;
    const y_draw_start = camera.pos.x + @intToFloat(f32, window.height) * y_camera_screen_pos * zoom;

    try drawRoad(&game.level, x_draw_start, y_draw_start, zoom);

    // TODO: just a temporary call so we can see something nifty
    // blit the pixels to the screen
    try game.window.render();
}

fn drawRoad(level: *Level, x_draw: f32, y_draw: f32, zoom: f32) !void {
    const inv_zoom = 1.0 / zoom;
    // TODO: row_bytes_skip is used to skip excess bytes in a row. For now
    // we are doing 16 bit graphics, so there are no bytes to skip.

    // the texture data is 16 bit, so these should really be []u16. But that would require endianness flipping,
    // and therefore an allocation.
    // an idea is to just read as bytes, then when I need the data to read in pairs, and handle endianness
    // via bitmasking, since we need to do that anyway to turn into an RBGA anyway.
    const background_texture = try packs.getEntryBytes(.textures_16, level.road_info.background_tex);
    const road_texture = try packs.getEntryBytes(.textures_16, level.road_info.foreground_tex);
    const left_border = try packs.getEntryBytes(.textures_16, level.road_info.road_left_border);
    const right_border = try packs.getEntryBytes(.textures_16, level.road_info.road_right_border);

    _ = x_draw;
    _ = y_draw;
    _ = inv_zoom;
    _ = background_texture;
    _ = road_texture;
    _ = left_border;
    _ = right_border;
}
