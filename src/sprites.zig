//! sprites.zig: functions for loading and drawing sprites

// Notes
// There are 300 slots for sprites in the original source, with an additional 100
// reserved for sprite copies (when sprites themselves are modified; I think this is how
// destruction is implemented, by copying and then actually distorting the pixels of the sprite)
// gSprites is global, but local to the sprites.c file

const std = @import("std");

const Allocator = std.mem.Allocator;

const packs = @import("packs.zig");

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
};

pub fn load(allocator: Allocator) ![]?Sprite {
    // TODO: color from prefs
    try packs.load(allocator, .sprites_16);
    defer packs.unload(allocator, .sprites_16);

    var sprites = try allocator.alloc(?Sprite, num_sprites + num_special_sprites);

    var index: usize = 0;
    while (index < num_sprites) : (index += 1) {
        if (packs.getEntryBytes(.sprites_16, @intCast(i16, index) + 128)) |data| {
            var reader = Reader.init(data);

            var sprite: Sprite = undefined;
            sprite.width = try reader.read(u16);
            sprite.height = try reader.read(u16);
            sprite.log2x_size = try reader.read(u8);
            try reader.skip(1);
            sprite.draw_mode = try reader.read(u8);
            try reader.skip(1);
            sprite.pixels = .{ .full = try reader.readSlice(
                u16,
                allocator,
                @intCast(usize, sprite.width) * @intCast(usize, sprite.height),
            ) };

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
