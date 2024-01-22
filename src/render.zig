//! render.zig: contains all code for drawing pixels to the screen!

const g = @import("g.zig");
const levels = @import("levels.zig");
const math = std.math;
const objects = @import("objects.zig");
const packs = @import("packs.zig");
const sprites = @import("sprites.zig");
const std = @import("std");
const trig = @import("trig.zig");
const window = @import("window.zig");

const Game = @import("game.zig").Game;
const Level = levels.Level;
const ObjectData = objects.ObjectData;
const Point = @import("point.zig").Point;
const RoadSegment = levels.RoadSegment;
const Slope = sprites.Slope;
const Sprite = sprites.Sprite;

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

    const pixels = game.window.pixels;
    try drawRoad(pixels, &game.level, x_draw_start, y_draw_start, zoom);
    try drawMarks(pixels, &game.level, x_draw_start, y_draw_start, zoom);
    try drawTracks(pixels, &game.level, game.frame_count, x_draw_start, y_draw_start, zoom);

    // drawSprites(pixels, game, camera, x_draw_start, y_draw_start, zoom);

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
        const world_y = math.clamp(y_draw - @as(f32, @floatFromInt(y)) * zoom, 0.0, @as(f32, @floatFromInt(level.road_data.len * 2)));
        const ceil_road_line = math.ceil(world_y * 0.5);
        const floor_road_line = math.floor(world_y * 0.5);
        var floor_perc = ceil_road_line - world_y * 0.5;

        const ceil_road = level.road_data[@intFromFloat(ceil_road_line)];
        const floor_road = level.road_data[@intFromFloat(floor_road_line)];

        const is_ceil_split = ceil_road[1] != ceil_road[2];
        const is_floor_split = floor_road[1] != floor_road[2];

        if (is_ceil_split != is_floor_split) {
            floor_perc = if (is_ceil_split) 1 else 0;
        } else if (is_ceil_split and is_floor_split) {
            if (ceil_road[3] < floor_road[2] or ceil_road[2] > floor_road[3]) floor_perc = 1;
        }

        var road_data: RoadSegment = undefined;
        road_data[0] = @as(i16, @intFromFloat(((floor_perc * @as(f32, @floatFromInt(floor_road[0])) + (1 - floor_perc) * @as(f32, @floatFromInt(ceil_road[0]))) - x_draw) * inv_zoom));
        road_data[1] = @as(i16, @intFromFloat(((floor_perc * @as(f32, @floatFromInt(floor_road[1])) + (1 - floor_perc) * @as(f32, @floatFromInt(ceil_road[1]))) - x_draw) * inv_zoom));
        road_data[2] = @as(i16, @intFromFloat(((floor_perc * @as(f32, @floatFromInt(floor_road[2])) + (1 - floor_perc) * @as(f32, @floatFromInt(ceil_road[2]))) - x_draw) * inv_zoom));
        road_data[3] = @as(i16, @intFromFloat(((floor_perc * @as(f32, @floatFromInt(floor_road[3])) + (1 - floor_perc) * @as(f32, @floatFromInt(ceil_road[3]))) - x_draw) * inv_zoom));

        drawRoadBorderLine(
            pixels,
            &draw_index,
            x_draw,
            math.minInt(i32),
            road_data[0],
            @intFromFloat(world_y),
            background_tex,
            left_border_tex,
            right_border_tex,
            zoom,
        );

        drawRoadLine(
            pixels,
            &draw_index,
            @intFromFloat(x_draw),
            road_data[0],
            road_data[1],
            @intFromFloat(world_y),
            road_tex,
            zoom,
        );

        drawRoadBorderLine(
            pixels,
            &draw_index,
            x_draw,
            road_data[1],
            road_data[2],
            @intFromFloat(world_y),
            background_tex,
            left_border_tex,
            right_border_tex,
            zoom,
        );

        drawRoadLine(
            pixels,
            &draw_index,
            @intFromFloat(x_draw),
            road_data[2],
            road_data[3],
            @intFromFloat(world_y),
            road_tex,
            zoom,
        );

        drawRoadBorderLine(
            pixels,
            &draw_index,
            x_draw,
            road_data[3],
            math.maxInt(i32),
            @intFromFloat(world_y),
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
    var left_border_end: i32 = @intFromFloat(@as(f32, @floatFromInt(x1)) + 16.0 / zoom);
    var right_border_end: i32 = blk: {
        const value = @as(f32, @floatFromInt(x2)) - 16.0 / zoom;
        if (value >= @as(f32, @floatFromInt(math.maxInt(i32)))) break :blk math.maxInt(i32);
        break :blk @intFromFloat(value);
    };
    if (left_border_end > right_border_end) {
        left_border_end = x1 + ((x2 - x1) >> 1);
        right_border_end = left_border_end;
    }

    drawRoadBorder(pixels, draw_index, x1, left_border_end, y, left_border_tex, zoom);
    drawRoadLine(pixels, draw_index, @intFromFloat(x_draw), left_border_end, right_border_end, y, data, zoom);
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
        u = @intFromFloat(@as(f32, @floatFromInt(-xpos_1)) * 256 * zoom);
        xpos_1 = 0;
    }
    if (xpos_2 > window.width) xpos_2 = window.width;
    if (xpos_2 < xpos_1) return;

    xpos_2 -= xpos_1;
    const data_offset: u32 = @intCast((-y & 0x007f) << 4);
    const dudx: u32 = @intFromFloat(zoom * 256);

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

    var u: u32 = @intCast((@as(i32, @intFromFloat(@as(f32, @floatFromInt(xpos_1)) * zoom + @as(f32, @floatFromInt(x_draw)))) & 0x007f) << 8);

    xpos_2 -= xpos_1;
    const data_offset: u32 = @intCast((-y & 0x007f) << 7);
    const dudx: u32 = @intFromFloat(zoom * 256);

    while (xpos_2 > 0) : (xpos_2 -= 1) {
        pixels[draw_index.*] = bigToNative(u16, data[data_offset + ((u >> 8) & 0x007f)]);
        u += dudx;
        draw_index.* += 1;
    }
}

const max_mark_len = 128;

fn drawMarks(pixels: []u16, level: *Level, x_draw: f32, y_draw: f32, zoom: f32) !void {
    const inv_zoom = 1.0 / zoom;
    const fix_zoom: u32 = @intFromFloat(zoom * 65536.0); // CHANGED i32 -> u32
    const size: i32 = @intFromFloat(4.2 * inv_zoom);

    // TODO: gFinishDelay
    const y1_clip: i32 = @intFromFloat(y_draw - window.height * zoom);

    const marks = level.marks;
    var mark_i = blk: {
        var l: u32 = 0;
        var r: u32 = @intCast(level.marks.len);

        // Some levels have no marks (l == 0 and r == 0) so prevent overflow
        while (r -| 1 > l) {
            const div = (l + r) / 2;
            if (marks[div].p1.y + marks[div].p2.y > y_draw * 2 + max_mark_len) l = div else r = div;
        }

        break :blk l;
    };

    const texture = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.marks);
    while (mark_i < level.marks.len and
        (marks[mark_i].p1.y + marks[mark_i].p2.y > @as(f32, @floatFromInt(y1_clip * 2 - max_mark_len)))) : (mark_i += 1)
    {
        if (marks[mark_i].p2.y <= y_draw + @as(f32, @floatFromInt(size)) and marks[mark_i].p1.y > @as(f32, @floatFromInt(y1_clip))) {
            const half_size: f32 = @floatFromInt(@divTrunc(size, 2));
            var x1 = (marks[mark_i].p1.x - x_draw) * inv_zoom - half_size;
            var x2 = (marks[mark_i].p2.x - x_draw) * inv_zoom - half_size;

            if ((x1 > @as(f32, @floatFromInt(-size)) or x2 > @as(f32, @floatFromInt(-size))) and (x1 < window.width or x2 < window.width)) {
                const y1 = (y_draw - marks[mark_i].p1.y) * inv_zoom - half_size;
                const y2 = (y_draw - marks[mark_i].p2.y) * inv_zoom - half_size;

                // TODO: v1 as u32 in original code, i32 here
                const @"u1" = @as(i32, @intFromFloat(marks[mark_i].p1.x)) << 16;
                const v1 = @as(u32, @intFromFloat(marks[mark_i].p1.y)) << 16;
                const @"u2" = @as(i32, @intFromFloat(marks[mark_i].p2.x)) << 16;
                // const v2 = @intCast(u32, @floatToInt(i32, marks[mark_i].p2.y) << 16); // TODO: unused?
                // std.debug.print("u1: {}\n", .{marks[mark_i].p1.x});

                if (y2 - y1 != 0) {
                    const dxdy: i32 = @intFromFloat((x2 - x1) / (y2 - y1) * 65536.0);
                    const dudy: i32 = @intFromFloat(@as(f32, @floatFromInt(@"u2" - @"u1")) / (y2 - y1));
                    var u = @"u1";
                    var v = v1;

                    var num_blocks = @as(i32, @intFromFloat(math.ceil(@abs((x2 - x1) / (y2 - y1)) - @as(f32, @floatFromInt(size)))));
                    if (num_blocks < 0) num_blocks = 0;
                    num_blocks += 1;

                    var x: i32 = @intFromFloat(x1 * 65536.0);
                    var y: i32 = @intFromFloat(y1);
                    // std.debug.print("y1: {} y2: {} x1: {} x2: {}\n", .{ y1, y2, x1, x2 });
                    while (@as(f32, @floatFromInt(y)) < y2) : (y += 1) {
                        var block_u = u;
                        var block_x = x >> 16;

                        var i: i32 = 0;
                        while (i < num_blocks) : (i += 1) {
                            if (block_x >= 0 and block_x < window.width - size and y >= 0 and y < window.height - size) {
                                // std.debug.print("num_blocks: {} {} {} {} {} {} {}\n", .{ num_blocks, block_x, y, block_u, v, fix_zoom, size });
                                drawTextureBlock(
                                    pixels,
                                    @intCast(block_x),
                                    @intCast(y),
                                    size,
                                    fix_zoom,
                                    block_u,
                                    v,
                                    texture,
                                );
                            } else {
                                // drawTextureBlockClipped(pixels, block_x, y, size, fix_zoom, @intCast(u32, block_u), v, texture);
                            }

                            block_u += @intCast(fix_zoom);
                            block_x += 1;
                        }

                        x += dxdy;
                        u += dudy;
                        v += fix_zoom;
                    }
                } else {
                    if (x1 > @as(f32, @floatFromInt(window.width - size))) x1 = @as(f32, @floatFromInt(window.width - size));
                    if (x2 < 0) x2 = 0;

                    if (x1 < x2) {
                        var u = @"u1";
                        if (x1 < 0) x1 = 0;
                        if (x2 > @as(f32, @floatFromInt(window.width - size))) x2 = @as(f32, @floatFromInt(window.width - size));
                        var x: i32 = @intFromFloat(x1);
                        while (@as(f32, @floatFromInt(x)) < x2) : (x += 1) {
                            u += @intCast(fix_zoom);
                            // drawTextureBlockClipped(pixels, x, @floatToInt(i32, y1), size, fix_zoom, @intCast(u32, u), v1, texture);
                        }
                    } else {
                        var u = @"u2";
                        var x: i32 = @intFromFloat(x2);
                        while (@as(f32, @floatFromInt(x)) < x1) : (x += 1) {
                            u += @intCast(fix_zoom);
                            // drawTextureBlockClipped(pixels, x, @floatToInt(i32, y1), size, fix_zoom, @intCast(u32, u), v1, texture);
                        }
                    }
                }
            }
        }
    }
}

/// Width of the tracks left by cars in pixels
const track_size = 3.4;

/// Time after which rubber tracks left by cars get removed in frames
const track_life_time = 14.0 * fps;

/// Time it takes for rubber tracks to fade out in frames
const track_death_duration = 0.8 * fps;

fn drawTracks(pixels: []u16, level: *Level, frame_count: u64, x_draw: f32, y_draw: f32, zoom: f32) !void {
    const inv_zoom = 1.0 / zoom;
    const fix_zoom: i32 = @intFromFloat(zoom * 65536.0);
    const size: i32 = @intFromFloat(track_size * inv_zoom);

    // TODO: gFinishDelay
    const y1_clip: i32 = @intFromFloat(y_draw - window.height * zoom);

    const textures = try packs.getEntrySliceNoAlloc(u16, .textures_16, level.road_info.tracks);

    for (0..g.track_count) |i| {
        const track = &g.tracks[i];
        if (track.p2.y <= y_draw + @as(f32, @floatFromInt(size))
                and track.p1.y > @as(f32, @floatFromInt(y1_clip))
                and @as(f32, @floatFromInt(track.time)) + track_life_time + track_death_duration > @as(f32, @floatFromInt(frame_count))) {
            var x2: f32 = (track.p2.x - x_draw) * inv_zoom - @as(f32, @floatFromInt(size)) / 2.0;
            var x1: f32 = (track.p1.x - x_draw) * inv_zoom - @as(f32, @floatFromInt(size)) / 2.0;

            const intensity: f32 = track.intensity * 3.0 * (if (@as(f32, @floatFromInt(track.time)) + track_life_time > @as(f32, @floatFromInt(frame_count))) 1 else 1 - (@as(f32, @floatFromInt(frame_count - track.time)) - track_life_time) / track_death_duration);

            const texture = textures[@intCast(@as(i32, @intFromFloat(intensity)) * 128 * 128)..];

            if ((x1 > @as(f32, @floatFromInt(-size)) or x2 > @as(f32, @floatFromInt(-size))) and (x1 < window.width or x2 < window.width)) {
                const y1: f32 = (y_draw - track.p1.y) * inv_zoom - @as(f32, @floatFromInt(size)) / 2.0;
                const y2: f32 = (y_draw - track.p2.y) * inv_zoom - @as(f32, @floatFromInt(size)) / 2.0;

                const v1: u32 = @intCast(@as(i32, @intFromFloat(track.p1.y)) << 16);
                const @"u1": i32 = @as(i32, @intFromFloat(track.p1.x)) << 16;
                const @"u2": i32 = @as(i32, @intFromFloat(track.p2.x)) << 16;

                if (y2 - y1 != 0) {
                    const dxdy: i32 = @intFromFloat((x2 - x1) / (y2 - y1) * 65536.0);
                    const dudy: i32 = @intFromFloat(@as(f32, @floatFromInt(@"u2" - @"u1")) / (y2 - y1));
                    var x: i32 = @intFromFloat(x1 * 65536.0);
                    var u: i32 = @"u1";
                    var v: i32 = @intCast(v1);

                    var num_blocks: i32 = @intFromFloat(@ceil(@abs((x2 - x1) / (y2 - y1)) - @as(f32, @floatFromInt(size))));
                    if (num_blocks < 0) num_blocks = 0;
                    num_blocks += 1;

                    for (@intFromFloat(y1)..@intFromFloat(y2)) |y| {
                        var block_u: i32 = u;
                        var block_x: i32 = x >> 16;

                        // In original code this is i again...
                        for (0..@intCast(num_blocks)) |j| {
                            _ = j;
                            if (block_x >= 0 and block_x < window.width - size and y >= 0 and y < window.height - size) {
                                drawTextureBlock(pixels, @intCast(block_x), @intCast(y), size, @intCast(fix_zoom), block_u, @intCast(v), texture);
                            } else {
                                drawTextureBlockClipped(pixels, block_x, @intCast(y), size, @intCast(fix_zoom), @intCast(block_u), @intCast(v), texture);
                            }

                            block_u += fix_zoom;
                            block_x += 1;
                        }

                        x += dxdy;
                        u += dudy;
                        v += fix_zoom;
                    }
                } else {
                    const win_minus_size: f32 = @floatFromInt(window.width - size);

                    if (x2 < 0) x2 = 0;
                    if (x1 > win_minus_size) x1 = win_minus_size;
                    if (x1 < x2) {
                        var u: i32 = @"u1";
                        if (x1 < 0) x1 = 0;
                        if (x2 > win_minus_size) x2 = win_minus_size;

                        for (@intFromFloat(x1)..@intFromFloat(x2)) |x| {
                            u += fix_zoom;
                            drawTextureBlockClipped(pixels, @intCast(x), @intFromFloat(y1), size, @intCast(fix_zoom), @intCast(u), v1, texture);
                        }
                    } else {
                        var u: i32 = @"u2";
                        if (x2 < 0) x2 = 0;
                        if (x1 > win_minus_size) x1 = win_minus_size;

                        for (@intFromFloat(x2)..@intFromFloat(x1)) |x| {
                            u += fix_zoom;
                            drawTextureBlockClipped(pixels, @intCast(x), @intFromFloat(y1), size, @intCast(fix_zoom), @intCast(u), v1, texture);
                        }
                    }
                }
            }
        }
    }
}

// TODO: these two could be simplified and sort of merged...
// TODO: u here should be u32
fn drawTextureBlock(pixels: []u16, x: u32, y: u32, size: i32, zoom: u32, u: i32, v: u32, texture: []const u16) void {
    var u_mut = u;
    var v_mut = v;

    var dst_index = y * window.width + x;

    var line: u32 = 0;
    while (line < size) : (line += 1) {
        const data_offset = (v_mut >> 9) & 0x3f80;

        var pix: u32 = 0;
        while (pix < size) : (pix += 1) {
            u_mut += @intCast(zoom);
            const pixel = texture[data_offset + @as(u32, @intCast(((u_mut >> 16) & 0x7f)))];
            pixels[dst_index] = bigToNative(u16, pixel);
            dst_index += 1;
        }

        v_mut += zoom;
        dst_index += window.width - @as(u32, @intCast(size));
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

    var dst_index: u32 = @intCast(y * window.row_bytes + x * 2);
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
        dst_index += window.row_bytes / 2 - @as(u32, @intCast(size));
    }
}

fn drawSprites(pixels: []u16, game: *Game, camera: ObjectData, x_draw: f32, y_draw: f32, zoom: f32) void {
    const level = &game.level;
    const inv_zoom = 1.0 / zoom;
    const tide = 0.05 * trig.sin(game.time * 2.6);
    const num_layers = @typeInfo(objects.ObjectLayers).Enum.fields.len;

    // Draw sprites layer by layer
    for (0..num_layers) |layer| drawSpriteLayer(pixels, level, game.sprites, camera, x_draw, y_draw, zoom, tide, @as(i32, @intCast(layer)));

    // Draw sprites that are jumping
    _ = inv_zoom;
}

fn drawSpriteLayer(pixels: []u16, level: *Level, all_sprites: []?Sprite, camera: ObjectData, x_draw: f32, y_draw: f32, zoom: f32, tide: f32, layer: i32) void {
    const inv_zoom = 1.0 / zoom;
    const max_draw_offs = 128.0 * inv_zoom;

    var obj = level.first_visible_ob;
    while (obj != level.last_visible_ob) : (obj = obj.next) {
        const obdata = obj.data;
        if (obdata.layer != layer) continue;
        if (obdata.frame == 0 or obdata.jump_height > 0.0) continue;

        const x = (obdata.pos.x - x_draw) * inv_zoom;
        const y = (y_draw - obdata.pos.y) * inv_zoom;

        if (y > -max_draw_offs and y < window.height + max_draw_offs) {
            const obj_tide = if (level.road_info.water != 0 and obdata.type.flag(.object_floating_flag))
                1.0 + tide * 0.5 + tide * obdata.velocity.value() * 0.04
            else
                1;

            drawSprite(pixels, all_sprites[@as(usize, @intCast(obdata.frame - 128))].?, x, y, obdata.dir, obj_tide * inv_zoom);

            // draw blurs
            const vel_diff = camera.velocity.diff(obdata.velocity);
            if ((vel_diff.x * vel_diff.x + vel_diff.y * vel_diff.y) > 35 * 35) {
                const fuzz_pos = vel_diff.scale(frame_duration * objects.pixels_per_meter * 0.2);
                drawSpriteTranslucent(pixels, all_sprites[@as(usize, @intCast(obdata.frame - 128))].?, x + fuzz_pos.x, y + fuzz_pos.y, obdata.dir - obdata.rot_vel * frame_duration, obj_tide * inv_zoom);
            }
        }
    }
}

// TODO: I'm keeping this as a global because it isn't clear if it needs to be...
var g_slopes: [window.height]Slope = undefined;

fn drawSprite(pixels: []u16, sprite: Sprite, cx: f32, cy: f32, dir: f32, z: f32) void {
    var zoom = z;
    if (sprite.mode(.double_size)) zoom *= 0.5;

    var dir_cos = trig.cos(dir);
    var dir_sin = trig.sin(dir);
    const dudx = @as(i32, @intFromFloat(dir_cos * 256.0 / zoom));
    const dvdx = @as(i32, @intFromFloat(-dir_sin * 256.0 / zoom));
    dir_cos *= zoom;
    dir_sin *= zoom;

    var y: i32 = 0;
    var y2: i32 = 0;
    if (sprite.mode(.transparent)) {
        if (sprites.slopeInit(&g_slopes, cx, cy, &y, &y2, dir_cos, dir_sin, sprite, dudx, dvdx)) {} else {}
    } else if (sprites.slopeInit(&g_slopes, cx, cy, &y, &y2, dir_cos, dir_sin, sprite, dudx, dvdx)) {} else {}

    _ = pixels;
}

fn drawSpriteTranslucent(pixels: []u16, sprite: Sprite, cx: f32, cy: f32, dir: f32, zoom: f32) void {
    _ = pixels;
    _ = sprite;
    _ = cx;
    _ = cy;
    _ = dir;
    _ = zoom;
}
