//! render.zig: contains all code for drawing pixels to the screen!

const levels = @import("levels.zig");
const packs = @import("packs.zig");
const std = @import("std");
const window = @import("window.zig");

const Game = @import("game.zig").Game;
const Level = levels.Level;
const RoadSegment = levels.RoadSegment;

const bigToNative = std.mem.bigToNative;

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
    const y_draw_start = camera.pos.y + @intToFloat(f32, window.height) * y_camera_screen_pos * zoom;

    try drawRoad(game.window.pixels, &game.level, x_draw_start, y_draw_start, zoom);

    // blit the pixels to the screen
    try game.window.render();
}

fn drawRoad(pixels: []u16, level: *Level, x_draw: f32, y_draw: f32, zoom: f32) !void {
    const inv_zoom = 1.0 / zoom;
    // TODO: row_bytes_skip is used to skip excess bytes in a row. For now
    // we are doing 16 bit graphics, so there are no bytes to skip.

    // The texture data is 16 bit, so these should really be []u16. But that
    // would require endianness flipping, and therefore an allocation. So the
    // getEntrySliceNoAlloc returns a []const u16. So we read textures, but
    // must remember to flip endianness when blitting to the pixels slice.
    const background_tex = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.background_tex);
    const road_tex = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.foreground_tex);
    const left_border_tex = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.road_left_border);
    const right_border_tex = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.road_right_border);

    var draw_index: u32 = 0;
    var y: i32 = 0;
    while (y < window.height) : (y += 1) {
        const world_y = std.math.clamp(y_draw - @intToFloat(f32, y) * zoom, 0.0, @intToFloat(f32, level.road_data.len * 2));
        const ceil_road_line = std.math.ceil(world_y * 0.5);
        const floor_road_line = std.math.floor(world_y * 0.5);
        var floor_perc = ceil_road_line - world_y * 0.5;

        var ceil_road = level.road_data[@floatToInt(usize, ceil_road_line)];
        var floor_road = level.road_data[@floatToInt(usize, floor_road_line)];

        const is_ceil_split = ceil_road[1] != ceil_road[2];
        const is_floor_split = floor_road[1] != floor_road[2];

        if (is_ceil_split != is_floor_split) {
            floor_perc = if (is_ceil_split) 1 else 0;
        } else if (is_ceil_split and is_floor_split) {
            if (ceil_road[3] < floor_road[2] or ceil_road[2] > floor_road[3]) floor_perc = 1;
        }

        var road_data: RoadSegment = undefined;
        road_data[0] = @floatToInt(
            i16,
            ((floor_perc * @intToFloat(f32, floor_road[0]) + (1 - floor_perc) * @intToFloat(f32, ceil_road[0])) - x_draw) * inv_zoom,
        );
        road_data[1] = @floatToInt(
            i16,
            ((floor_perc * @intToFloat(f32, floor_road[1]) + (1 - floor_perc) * @intToFloat(f32, ceil_road[1])) - x_draw) * inv_zoom,
        );
        road_data[2] = @floatToInt(
            i16,
            ((floor_perc * @intToFloat(f32, floor_road[2]) + (1 - floor_perc) * @intToFloat(f32, ceil_road[2])) - x_draw) * inv_zoom,
        );
        road_data[3] = @floatToInt(
            i16,
            ((floor_perc * @intToFloat(f32, floor_road[3]) + (1 - floor_perc) * @intToFloat(f32, ceil_road[3])) - x_draw) * inv_zoom,
        );

        drawRoadBorderLine(
            pixels,
            &draw_index,
            x_draw,
            std.math.minInt(i32),
            road_data[0],
            @floatToInt(i32, world_y),
            background_tex,
            left_border_tex,
            right_border_tex,
            zoom,
        );

        drawRoadLine(
            pixels,
            &draw_index,
            @floatToInt(i32, x_draw),
            road_data[0],
            road_data[1],
            @floatToInt(i32, world_y),
            road_tex,
            zoom,
        );

        drawRoadBorderLine(
            pixels,
            &draw_index,
            x_draw,
            road_data[1],
            road_data[2],
            @floatToInt(i32, world_y),
            background_tex,
            left_border_tex,
            right_border_tex,
            zoom,
        );

        drawRoadLine(
            pixels,
            &draw_index,
            @floatToInt(i32, x_draw),
            road_data[2],
            road_data[3],
            @floatToInt(i32, world_y),
            road_tex,
            zoom,
        );

        drawRoadBorderLine(
            pixels,
            &draw_index,
            x_draw,
            road_data[3],
            std.math.maxInt(i32),
            @floatToInt(i32, world_y),
            background_tex,
            left_border_tex,
            right_border_tex,
            zoom,
        );
    }
}

fn drawRoadBorderLine(
    pixels: []u16,
    draw_index: *u32,
    x_draw: f32,
    x1: i32,
    x2: i32,
    y: i32,
    data: []const u16,
    left_border_tex: []const u16,
    right_border_tex: []const u16,
    zoom: f32,
) void {
    var left_border_end = @floatToInt(i32, @intToFloat(f32, x1) + 16.0 / zoom);
    var right_border_end = blk: {
        const value = @intToFloat(f32, x2) - 16.0 / zoom;
        if (value >= @intToFloat(f32, std.math.maxInt(i32))) break :blk std.math.maxInt(i32);
        break :blk @floatToInt(i32, value);
    };
    if (left_border_end > right_border_end) {
        left_border_end = x1 + ((x2 - x1) >> 1);
        right_border_end = left_border_end;
    }

    drawRoadBorder(pixels, draw_index, x1, left_border_end, y, left_border_tex, zoom);
    drawRoadLine(pixels, draw_index, @floatToInt(i32, x_draw), left_border_end, right_border_end, y, data, zoom);
    drawRoadBorder(pixels, draw_index, right_border_end, x2, y, right_border_tex, zoom);
}

fn drawRoadBorder(
    pixels: []u16,
    draw_index: *u32,
    x1: i32,
    x2: i32,
    y: i32,
    data: []const u16,
    zoom: f32,
) void {
    var xpos_1 = x1;
    var xpos_2 = x2;
    if (xpos_2 < 0) return;

    var u: u32 = 0;
    if (xpos_1 < 0) {
        u = @floatToInt(u32, @intToFloat(f32, -xpos_1) * 256 * zoom);
        xpos_1 = 0;
    }
    if (xpos_2 > window.width) xpos_2 = window.width;
    if (xpos_2 < xpos_1) return;

    xpos_2 -= xpos_1;
    const data_offset = @intCast(u32, (-y & 0x007f) << 4);
    const dudx = @floatToInt(u32, zoom * 256);

    while (xpos_2 > 0) : (xpos_2 -= 1) {
        pixels[draw_index.*] = bigToNative(u16, data[data_offset + (u >> 8)]);
        u += dudx;
        draw_index.* += 1;
    }
}

// TODO: xdrift ydrift
fn drawRoadLine(
    pixels: []u16,
    draw_index: *u32,
    x_draw: i32,
    x1: i32,
    x2: i32,
    y: i32,
    data: []const u16,
    zoom: f32,
) void {
    var xpos_1 = x1;
    var xpos_2 = x2;
    if (xpos_1 < 0) xpos_1 = 0;
    if (xpos_2 > window.width) xpos_2 = window.width;
    if (xpos_2 < xpos_1) return;

    var u = @intCast(u32, (@floatToInt(i32, @intToFloat(f32, xpos_1) * zoom + @intToFloat(f32, x_draw)) & 0x007f) << 8);

    xpos_2 -= xpos_1;
    const data_offset = @intCast(u32, (-y & 0x007f) << 7);
    const dudx = @floatToInt(u32, zoom * 256);

    while (xpos_2 > 0) : (xpos_2 -= 1) {
        pixels[draw_index.*] = bigToNative(u16, data[data_offset + ((u >> 8) & 0x007f)]);
        u += dudx;
        draw_index.* += 1;
    }
}
