//! game.zig: the core of the game logic
//! Contains code for initialization and cleanup of game resources
//! and the main game loop.

const std = @import("std");
const time = std.time;

const Allocator = std.mem.Allocator;

const levels = @import("levels.zig");
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
    level: levels.Level = undefined,
};

const Player = struct {
    lives: u8 = 3,
    extra_lives: u8 = 0,
    addons: u32 = 0,
    death_delay: f32 = 0,
    score: i32 = 0,
};

fn initData(allocator: Allocator) !void {
    // load packs
    try packs.load(allocator, .sounds);
    try packs.load(allocator, .object_type);
    try packs.load(allocator, .object_groups);
    try packs.load(allocator, .road);

    // TODO: disable full color based on prefs
    // if implemented, then this defer may free already freed memory
    try packs.load(allocator, .rle_16);
    try packs.load(allocator, .crle_16);
    try packs.load(allocator, .textures_16);

    // Initialize object type map
    try objects.initObjectTypes(allocator);
}

fn deinitData(allocator: Allocator) void {
    defer packs.unload(allocator, .sounds);
    defer packs.unload(allocator, .object_type);
    defer packs.unload(allocator, .object_groups);
    defer packs.unload(allocator, .road);
    defer packs.unload(allocator, .rle_16);
    defer packs.unload(allocator, .crle_16);
    defer packs.unload(allocator, .textures_16);
    defer objects.deinitObjectTypes();
}

/// Run the game
pub fn start(allocator: Allocator) !void {
    // initialize PRNG
    random.init(@bitCast(u64, time.timestamp()));

    // load packs and other constant game resources
    try initData(allocator);
    defer deinitData(allocator);

    // TODO: load preferences & check registration

    var game: Game = .{};

    // load sprites into memory
    game.sprites = try sprites.load(allocator);
    defer sprites.unload(allocator, game.sprites);

    game.level = try levels.load(allocator, .level_01);
    defer game.level.deinit(allocator);

    // Create player object
    // TODO: use constants for player car ID
    // TODO: use bool for water?
    var player = try objects.create(allocator, if (game.level.road_info.water == 1) 201 else 128);
    player.pos.x = @intToFloat(f32, game.level.header.x_start);
    player.pos.y = 500.0;
    player.control = .drive_up;
    player.target = 1;
    try game.level.objects.append(player);
}
