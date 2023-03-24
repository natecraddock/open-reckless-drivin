//! game.zig: the core of the game logic
//! Contains code for initialization and cleanup of game resources
//! and the main game loop.

const levels = @import("levels.zig");
const objects = @import("objects.zig");
const packs = @import("packs.zig");
const random = @import("random.zig");
const render = @import("render.zig");
const sprites = @import("sprites.zig");
const std = @import("std");
const time = std.time;
const utils = @import("utils.zig");

const Allocator = std.mem.Allocator;
const Object = objects.Object;
const Sprite = sprites.Sprite;
const Window = @import("window.zig").Window;

pub const Game = struct {
    allocator: Allocator,

    window: Window = undefined,
    state: enum {
        menu,
        game,
    } = .game,
    player: Player = .{},
    sprites: []?Sprite = undefined,
    level: levels.Level = undefined,
    start_time: u64 = 0,
    frame_count: u64 = 0,
    zoom_vel: f32 = 80.0,

    /// Free a sprite's resources if it was created as a special sprite
    pub fn specialSpriteUnused(self: *Game, id: i32) void {
        const index = @intCast(usize, id - 128);
        if (index < sprites.num_sprites) return;
        if (self.sprites[index]) |sprite| {
            self.allocator.free(sprite.pixels.full);
            self.sprites[index] = null;
        }
    }
};

const Player = struct {
    obj: *Object = undefined,
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
    var game: Game = .{ .allocator = allocator };

    // initialize PRNG
    random.init(@bitCast(u64, time.timestamp()));

    // load packs and other constant game resources
    try initData(allocator);
    defer deinitData(allocator);

    // TODO: load preferences & check registration

    // load sprites into memory
    game.sprites = try sprites.load(allocator);
    defer sprites.unload(allocator, game.sprites);

    // load the first level
    game.level = try levels.load(allocator, .level_01);
    defer game.level.deinit(allocator);

    // Create player object
    // TODO: use constants for player car ID
    // TODO: use bool for water?
    var node = try objects.create(allocator, if (game.level.road_info.water == 1) 201 else 128);
    var player = &node.data;
    player.pos.x = @intToFloat(f32, game.level.header.x_start);
    player.pos.y = 500.0;
    player.control = .drive_up;
    player.target = 1;
    player.is_player = true;
    game.player.obj = node;
    game.level.objects.append(node);

    game.start_time = getMicroTime();

    // Finally initialize the window and start the gameloop
    game.window = try Window.init(allocator);
    defer game.window.deinit(allocator);
    try gameloop(&game);
}

/// The main gameloop of Reckless Drivin'
fn gameloop(game: *Game) !void {
    while (game.state == .game) {
        objects.update(game, &game.level);
        // player handling

        // just to see things scrolling about :)
        // game.player.obj.data.pos.y += 15;

        game.frame_count += 1;

        // event handling (move elsewhere later?)
        while (Window.getEvent()) |ev| {
            switch (ev) {
                else => {},
            }
        }

        if (checkFrameTime(game)) {
            try render.renderFrame(game);
        }

        if (Window.shouldClose()) break;
    }
}

const frames_per_microsecond = render.fps / 1000000.0;

/// Return the current time in microseconds
fn getMicroTime() u64 {
    return @intCast(u64, @divTrunc(time.nanoTimestamp(), 1000));
}

/// Check if a frame should be rendered
/// TODO: this seems to always return true, and might be a NOP. Return to this
/// at a later point to investigate...
fn checkFrameTime(game: *Game) bool {
    const current_time = getMicroTime() - game.start_time;
    const optimal_frame_count = @intToFloat(f32, current_time) * frames_per_microsecond;

    return @intToFloat(f32, game.frame_count) > optimal_frame_count;
}
