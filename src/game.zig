//! game.zig: the core of the game logic
//! Contains code for initialization and cleanup of game resources
//! and the main game loop.

const std = @import("std");
const time = std.time;

const Allocator = std.mem.Allocator;

const packs = @import("packs.zig");
const random = @import("random.zig");
const sprites = @import("sprites.zig");

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

    // load sprites into memory
    // idea: use sprites.load() to create an ArrayList(Sprite) of all sprites
    // then store in a game struct {} that stores any data needed (and allocator?)
    // that would reduce the number of things to pass to each function :)
    // sprites are _almost_ read-only data, but the pixels themselves are modified
    // for bullet hit effects annoyingly :|
    var sprites_data = try sprites.load(allocator);
    defer sprites.unload(allocator, sprites_data);
}
