//! game.zig: the core of the game logic
//! Contains code for initialization and cleanup of game resources
//! and the main game loop.

const std = @import("std");
const time = std.time;

const Allocator = std.mem.Allocator;

const packs = @import("packs.zig");
const random = @import("random.zig");

/// Initialize basic game resources
fn init(allocator: Allocator) !void {
    // initialize PRNG
    random.init(@bitCast(u64, time.timestamp()));

    // TODO: load preferences & check registration

    // load packs
    try packs.load(allocator, .sounds);
    try packs.load(allocator, .object_type);
    try packs.load(allocator, .object_groups);
    try packs.load(allocator, .road);
    // TODO: disable full color?
    try packs.load(allocator, .rle_16);
    try packs.load(allocator, .crle_16);
    try packs.load(allocator, .textures_16);
}

/// Cleanup game resources
fn deinit(allocator: Allocator) void {
    packs.unload(allocator, .sounds);
    packs.unload(allocator, .object_type);
    packs.unload(allocator, .object_groups);
    packs.unload(allocator, .road);
    // TODO: disable full color?
    packs.unload(allocator, .rle_16);
    packs.unload(allocator, .crle_16);
    packs.unload(allocator, .textures_16);
}

/// Run the game
pub fn start(allocator: Allocator) !void {
    try init(allocator);
    defer deinit(allocator);
}
