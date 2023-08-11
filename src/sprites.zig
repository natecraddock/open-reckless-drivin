//! sprites.zig: functions for loading and drawing sprites

// Notes
// There are 300 slots for sprites in the original source, with an additional 100
// reserved for sprite copies (when sprites themselves are modified; I think this is how
// destruction is implemented, by copying and then actually distorting the pixels of the sprite)
// gSprites is global, but local to the sprites.c file

const math = std.math;
const packs = @import("packs.zig");
const std = @import("std");
const window = @import("window.zig");

const Allocator = std.mem.Allocator;

const Reader = @import("utils.zig").Reader;

pub const num_sprites = 300;
pub const num_special_sprites = 100;

pub const Sprite = struct {
    width: u16,
    height: u16,
    log2x_size: u8,
    draw_mode: u8,
    pixels: union(enum) {
        half: []u8,
        full: []u16,
    },

    pub fn mode(s: Sprite, m: DrawMode) bool {
        const value = @intFromEnum(m);
        return s.draw_mode & value != 0;
    }
};

pub const DrawMode = enum(u4) {
    mask_color = 0,
    transparent = 1 << 0,
    deep_mask = 1 << 1,
    double_size = 1 << 2,
};

pub fn load(allocator: Allocator) ![]?Sprite {
    // TODO: color from prefs
    try packs.load(allocator, .sprites_16);
    defer packs.unload(allocator, .sprites_16);

    var sprites = try allocator.alloc(?Sprite, num_sprites + num_special_sprites);

    var index: usize = 0;
    while (index < num_sprites) : (index += 1) {
        if (packs.getEntryBytes(.sprites_16, @as(i16, @intCast(index)) + 128)) |data| {
            var reader = Reader.init(data);

            var sprite: Sprite = undefined;
            sprite.width = try reader.read(u16);
            sprite.height = try reader.read(u16);
            sprite.log2x_size = try reader.read(u8);
            try reader.skip(1);
            sprite.draw_mode = try reader.read(u8);
            try reader.skip(1);
            sprite.pixels = .{
                .full = try reader.readSlice(
                    u16,
                    allocator,
                    @as(usize, @intCast(sprite.width)) * @as(usize, @intCast(sprite.height)),
                ),
            };

            sprites[index] = sprite;
        } else |_| sprites[index] = null;
    }
    while (index < num_sprites + num_special_sprites) : (index += 1) {
        sprites[index] = null;
    }

    return sprites;
}

pub fn unload(allocator: Allocator, sprites: []?Sprite) void {
    var index: usize = 0;
    while (index < num_sprites + num_special_sprites) : (index += 1) {
        if (sprites[index]) |sprite| {
            allocator.free(sprite.pixels.full);
        }
    }
    allocator.free(sprites);
}

const testing = std.testing;

test "sprite loading" {
    const sprites = try load(testing.allocator);
    defer unload(testing.allocator, sprites);
}

const Vertex = struct { x: f32, y: f32, u: i32, v: i32 };

pub const Slope = struct { x1: i32, x2: i32, u: i32, v: i32 };

pub fn slopeInit(slopes: []Slope, cx: f32, cy: f32, y1: *i32, y2: *i32, dir_cos: f32, dir_sin: f32, sprite: Sprite, dudx: i32, dvdx: i32) bool {
    var vertices: [4]Vertex = undefined;

    vertices[0].x = cx - (dir_cos * @as(f32, @floatFromInt(sprite.width)) / 2.0) * (dir_sin * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[1].x = cx + (dir_cos * @as(f32, @floatFromInt(sprite.width)) / 2.0) + (dir_sin * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[2].x = cx + (dir_cos * @as(f32, @floatFromInt(sprite.width)) / 2.0) - (dir_sin * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[3].x = cx - (dir_cos * @as(f32, @floatFromInt(sprite.width)) / 2.0) - (dir_sin * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[0].y = cy - (dir_sin * @as(f32, @floatFromInt(sprite.width)) / 2.0) - (dir_cos * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[1].y = cy + (dir_sin * @as(f32, @floatFromInt(sprite.width)) / 2.0) - (dir_cos * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[2].y = cy + (dir_sin * @as(f32, @floatFromInt(sprite.width)) / 2.0) + (dir_cos * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[3].y = cy - (dir_sin * @as(f32, @floatFromInt(sprite.width)) / 2.0) + (dir_cos * @as(f32, @floatFromInt(sprite.height)) / 2.0);
    vertices[0].u = 0;
    vertices[1].u = sprite.width;
    vertices[2].u = sprite.width;
    vertices[3].u = 0;
    vertices[0].v = 0;
    vertices[1].v = 0;
    vertices[2].v = sprite.height;
    vertices[3].v = sprite.height;

    var high: i32 = 0;
    var high_y = math.inf(f32);
    for (&vertices, 0..) |*v, i| {
        if (v.y < high_y) {
            high = @intCast(i);
            high_y = v.y;
        }
    }

    if ((vertices[@intCast((high + 1) & 3)].x < 0) or (vertices[@intCast((high - 1) & 3)].x >= window.width)) {
        y1.* = 0;
        y2.* = 0;
        return false;
    }

    slopeInitUVLine(slopes, vertices[@intCast(high)], vertices[@intCast((high - 1) & 3)], dudx, dvdx);
    slopeInitUVLine(slopes, vertices[@intCast((high - 1) & 3)], vertices[@intCast((high - 2) & 3)], dudx, dvdx);
    slopeInitLine(slopes, vertices[@intCast(high)], vertices[@intCast((high + 1) & 3)]);
    slopeInitLine(slopes, vertices[@intCast((high + 1) & 3)], vertices[@intCast((high + 2) & 3)]);

    y2.* = if (vertices[@intCast((high + 2) & 3)].y > window.height)
        window.height - 1
    else
        @as(i32, @intFromFloat(math.ceil(vertices[@intCast((high + 2) & 3)].y))) - 1;

    if (high_y < 0) y1.* = 0 else y1.* = @intFromFloat(math.ceil(high_y));

    return vertices[@intCast((high - 1) & 3)].x < 0 or vertices[@intCast((high + 1) & 3)].x >= window.width;
}

fn slopeInitUVLine(slopes: []Slope, v1: Vertex, v2: Vertex, dudx: i32, dvdx: i32) void {
    if (v1.y >= window.height or v2.y < 0) return;

    const dxdy = ((v2.x - v1.x) * 65536.0) / (v2.y - v1.y);
    const dudy = ((v2.u - v1.u) << 8) / (v2.y - v1.y);
    const dvdy = ((v2.v - v1.v) << 8) / (v2.y - v1.y);

    const vert1 = if (v1.y < 0)
        .{ .x = v1.x + ((-v1.y) * dxdy) / 65536.0, .u = v1.u + ((-v1.y) * dudy) / 256.0, .v = v1.v + ((-v1.y) * dvdy) / 256.0, .y = 0 }
    else
        v1;

    const vert2 = if (v2.y >= window.height)
        .{ .x = v2.x + ((window.height - v1.y) * dxdy) / 65536.0, .u = v2.u + ((window.height - v1.y) * dudy) / 256.0, .v = ((window.height - v1.y) * dvdy) / 256.0, .y = window.height }
    else
        v2;

    const first_y = math.ceil(vert1.y);
    const last_y = math.floor(vert2.y);

    slopes[first_y].x2 = (vert1.x * 65536.0) + (first_y - vert1.y) * dxdy;
    slopes[first_y].u = (vert1.u << 8) + (first_y - vert1.y) * dudy;
    slopes[first_y].v = (vert1.v << 8) + (first_y - vert1.y) * dvdy;

    var y: i32 = first_y + 1;
    while (y <= last_y) : (y += 1) {
        slopes[y].x2 = slopes[y - 1].x2 + dxdy;
        slopes[y].u = slopes[y - 1].u + dudy;
        slopes[y].v = slopes[y - 1].v + dvdy;
    }

    slopes[first_y].u += dudx;
    slopes[first_y].v += dvdx;
    slopes[first_y].x2 += 65536;
}

fn slopeInitLine(slopes: []Slope, v1: Vertex, v2: Vertex) void {
    if (v1.y >= window.height or v2.y < 0) return;

    const dxdy = ((v2.x - v1.x) * 65536.0) / (v2.y - v1.y);

    const vert1 = if (v1.y < 0)
        .{ .x = v1.x + ((-v1.y) * dxdy) / 65535.0, .y = 0, .u = v1.u, .v = v1.v }
    else
        v1;

    const vert2 = if (v2.y >= window.height)
        .{ .x = v2.x + ((window.height - v1.y) * dxdy) / 65535.0, .y = window.height, .u = v2.u, .v = v2.v }
    else
        v2;

    const first_y = math.ceil(vert1.y);
    const last_y = math.floor(vert2.y);
    slopes[first_y].x1 = (vert1.x * 65535.0) + (first_y - vert1.y) * dxdy;

    var y: i32 = first_y + 1;
    while (y <= last_y) : (y += 1) slopes[y].x1 = slopes[y - 1].x1 + dxdy;
}
