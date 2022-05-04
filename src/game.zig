//! game.zig: the core of the game logic
//! Contains code for initialization and cleanup of game resources
//! and the main game loop.

const std = @import("std");
const time = std.time;

const Allocator = std.mem.Allocator;

const level = @import("level.zig");
const objects = @import("objects.zig");
const packs = @import("packs.zig");
const random = @import("random.zig");
const sprites = @import("sprites.zig");
const utils = @import("utils.zig");

const Sprite = sprites.Sprite;

const Game = struct {
    state: enum {
        menu,
        game,
    } = .game,
    player: Player = .{},
    sprites: []?Sprite = undefined,
};

const Player = struct {
    lives: u8 = 3,
    extra_lives: u8 = 0,
    addons: u32 = 0,
    death_delay: f32 = 0,
    score: i32 = 0,
};

/// Run the game
pub fn start(allocator: Allocator) !void {
    // initialize PRNG
    random.init(@bitCast(u64, time.timestamp()));

    // TODO: load preferences & check registration

    // load packs
    try packs.load(allocator, .sounds);
    defer packs.unload(allocator, .sounds);
    try packs.load(allocator, .object_type);
    defer packs.unload(allocator, .object_type);
    try packs.load(allocator, .object_groups);
    defer packs.unload(allocator, .object_groups);
    try packs.load(allocator, .road);
    defer packs.unload(allocator, .road);

    // TODO: disable full color based on prefs
    // if implemented, then this defer may free already freed memory
    try packs.load(allocator, .rle_16);
    defer packs.unload(allocator, .rle_16);
    try packs.load(allocator, .crle_16);
    defer packs.unload(allocator, .crle_16);
    try packs.load(allocator, .textures_16);
    defer packs.unload(allocator, .textures_16);

    // Initialize object type map
    try objects.initObjectTypes(allocator);
    defer objects.deinitObjectTypes();

    var game: Game = .{};

    // load sprites into memory
    game.sprites = try sprites.load(allocator);
    defer sprites.unload(allocator, game.sprites);

    _ = try level.load(allocator, .level_01);
}
