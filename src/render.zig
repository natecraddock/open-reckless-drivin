//! render.zig: contains all code for drawing pixels to the screen!

const levels = @import("levels.zig");
const math = std.math;
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
    const x_draw_start = camera.pos.x - window.width * x_camera_screen_pos * zoom;
    const y_draw_start = camera.pos.y + window.height * y_camera_screen_pos * zoom;

    if (true) return;

    var pixels = game.window.pixels;
    try drawRoad(pixels, &game.level, x_draw_start, y_draw_start, zoom);
    try drawMarks(pixels, &game.level, x_draw_start, y_draw_start, zoom);

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
        const world_y = math.clamp(y_draw - @intToFloat(f32, y) * zoom, 0.0, @intToFloat(f32, level.road_data.len * 2));
        const ceil_road_line = math.ceil(world_y * 0.5);
        const floor_road_line = math.floor(world_y * 0.5);
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
            math.minInt(i32),
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
            math.maxInt(i32),
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
        if (value >= @intToFloat(f32, math.maxInt(i32))) break :blk math.maxInt(i32);
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

const max_mark_len = 128;

fn drawMarks(pixels: []u16, level: *Level, x_draw: f32, y_draw: f32, zoom: f32) !void {
    const inv_zoom = 1.0 / zoom;
    const fix_zoom = @floatToInt(u32, zoom * 65536.0); // CHANGED i32 -> u32
    const size = @floatToInt(i32, 4.2 * inv_zoom);

    const y1_clip = @floatToInt(i32, y_draw - window.height * zoom);

    var marks = level.marks;
    var mark_i = blk: {
        var l: u32 = 0;
        var r: u32 = @intCast(u32, level.marks.len);

        // Some levels have no marks (l == 0 and r == 0) so prevent overflow
        while (r -| 1 > l) {
            const div = (l + r) / 2;
            if (marks[div].p1.y + marks[div].p2.y > y_draw * 2 + max_mark_len) l = div else r = div;
        }

        break :blk l;
    };

    const texture = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.marks);
    while (mark_i < level.marks.len and
        (marks[mark_i].p1.y + marks[mark_i].p2.y > @intToFloat(f32, y1_clip * 2 - max_mark_len))) : (mark_i += 1)
    {
        if (marks[mark_i].p2.y <= y_draw + @intToFloat(f32, size) and marks[mark_i].p1.y > @intToFloat(f32, y1_clip)) {
            const half_size = @intToFloat(f32, @divTrunc(size, 2));
            var x1 = (marks[mark_i].p1.x - x_draw) * inv_zoom - half_size;
            var x2 = (marks[mark_i].p2.x - x_draw) * inv_zoom - half_size;

            if ((x1 > @intToFloat(f32, -size) or x2 > @intToFloat(f32, -size)) and (x1 < window.width or x2 < window.width)) {
                var y1 = (y_draw - marks[mark_i].p1.y) * inv_zoom - half_size;
                var y2 = (y_draw - marks[mark_i].p2.y) * inv_zoom - half_size;

                // TODO: v1 as u32 in original code, i32 here
                const @"u1" = @floatToInt(i32, marks[mark_i].p1.x) << 16;
                const v1 = @floatToInt(u32, marks[mark_i].p1.y) << 16;
                const @"u2" = @floatToInt(i32, marks[mark_i].p2.x) << 16;
                // const v2 = @intCast(u32, @floatToInt(i32, marks[mark_i].p2.y) << 16); // TODO: unused?
                // std.debug.print("u1: {}\n", .{marks[mark_i].p1.x});

                if (y2 - y1 != 0) {
                    const dxdy = @floatToInt(i32, (x2 - x1) / (y2 - y1) * 65536.0);
                    const dudy = @floatToInt(i32, @intToFloat(f32, @"u2" - @"u1") / (y2 - y1));
                    var u = @"u1";
                    var v = v1;

                    var num_blocks = @floatToInt(i32, math.ceil(math.fabs((x2 - x1) / (y2 - y1)) - @intToFloat(f32, size)));
                    if (num_blocks < 0) num_blocks = 0;
                    num_blocks += 1;

                    var x = @floatToInt(i32, x1 * 65536.0);
                    var y = @floatToInt(i32, y1);
                    // std.debug.print("y1: {} y2: {} x1: {} x2: {}\n", .{ y1, y2, x1, x2 });
                    while (@intToFloat(f32, y) < y2) : (y += 1) {
                        var block_u = u;
                        var block_x = x >> 16;

                        var i: i32 = 0;
                        while (i < num_blocks) : (i += 1) {
                            if (block_x >= 0 and block_x < window.width - size and y >= 0 and y < window.height - size) {
                                // std.debug.print("num_blocks: {} {} {} {} {} {} {}\n", .{ num_blocks, block_x, y, block_u, v, fix_zoom, size });
                                drawTextureBlock(
                                    pixels,
                                    @intCast(u32, block_x),
                                    @intCast(u32, y),
                                    size,
                                    fix_zoom,
                                    block_u,
                                    v,
                                    texture,
                                );
                            } else {
                                // drawTextureBlockClipped(pixels, block_x, y, size, fix_zoom, @intCast(u32, block_u), v, texture);
                            }

                            block_u += @intCast(i32, fix_zoom);
                            block_x += 1;
                        }

                        x += dxdy;
                        u += dudy;
                        v += fix_zoom;
                    }
                } else {
                    if (x1 > @intToFloat(f32, window.width - size)) x1 = @intToFloat(f32, window.width - size);
                    if (x2 < 0) x2 = 0;

                    if (x1 < x2) {
                        var u = @"u1";
                        if (x1 < 0) x1 = 0;
                        if (x2 > @intToFloat(f32, window.width - size)) x2 = @intToFloat(f32, window.width - size);
                        var x = @floatToInt(i32, x1);
                        while (@intToFloat(f32, x) < x2) : (x += 1) {
                            u += @intCast(i32, fix_zoom);
                            // drawTextureBlockClipped(pixels, x, @floatToInt(i32, y1), size, fix_zoom, @intCast(u32, u), v1, texture);
                        }
                    } else {
                        var u = @"u2";
                        var x = @floatToInt(i32, x2);
                        while (@intToFloat(f32, x) < x1) : (x += 1) {
                            u += @intCast(i32, fix_zoom);
                            // drawTextureBlockClipped(pixels, x, @floatToInt(i32, y1), size, fix_zoom, @intCast(u32, u), v1, texture);
                        }
                    }
                }
            }
        }
    }
}

// TODO: these two could be simplified and sort of merged...
fn drawTextureBlock(pixels: []u16, x: u32, y: u32, size: i32, zoom: u32, u: i32, v: u32, texture: []const u16) void {
    var u_mut = u;
    var v_mut = v;

    var dst_index = y * window.width + x;

    var line: u32 = 0;
    while (line < size) : (line += 1) {
        const data_offset = (v_mut >> 9) & 0x3f80;

        var pix: u32 = 0;
        while (pix < size) : (pix += 1) {
            u_mut += @intCast(i32, zoom);
            const pixel = texture[data_offset + @intCast(u32, ((u_mut >> 16) & 0x7f))];
            pixels[dst_index] = bigToNative(u16, pixel);
            dst_index += 1;
        }

        v_mut += zoom;
        dst_index += window.width - @intCast(u32, size);
    }
}

fn drawTextureBlockClipped(pixels: []u16, x: i32, y: i32, size: i32, zoom: u32, u: u32, v: u32, texture: []const u16) void {
    var x_size = size;
    var y_size = size;
    var x_mut = x;
    var y_mut = y;
    var u_mut = u;
    var v_mut = v;
    if (x < 0) {
        x_size += x;
        x_mut = 0;
    }
    if (y < 0) {
        y_size += y;
        y_mut = 0;
    }
    if (x_mut + size > window.width) x_size += window.width - x_mut - size;
    if (y_mut + size > window.height) y_size += window.height - y_mut - size;

    var dst_index = @intCast(u32, y * window.row_bytes + x * 2);
    var line: u32 = 0;
    while (line < y_size) : (line += 1) {
        const data_offset = (v_mut >> 9) & 0x3f80;

        var pix: u32 = 0;
        while (pix < x_size) : (pix += 1) {
            u_mut += zoom;
            const pixel = texture[data_offset + ((u_mut >> 16) & 0x7f)];
            pixels[dst_index] = bigToNative(u16, pixel);
            dst_index += 1;
        }
        v_mut += zoom;
        dst_index += window.row_bytes / 2 - @intCast(u32, size);
    }
}
