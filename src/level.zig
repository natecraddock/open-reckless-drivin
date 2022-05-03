//! level.zig: for managing the data for each game level

const std = @import("std");

const Allocator = std.mem.Allocator;

const packs = @import("packs.zig");

const Point = @import("point.zig").Point;

const Level = packed struct {
    road_info_entry: i16,
    time: u16,
    object_groups: [10]ObjectGroupRef,
    x_start: i16,
    level_end: u16,
};

const ObjectGroupRef = packed struct { id: i16, num: i16 };

const Mark = packed struct { p1: Point, p2: Point };

const RoadInfo = packed struct {
    friction: f32,
    air_resistance: f32,
    back_resistance: f32, // obsolete
    tolerance: u16,
    marks: i16,
    death_offs: i16,
    background_tex: i16,
    foreground_tex: i16,
    road_left_border: i16,
    road_right_border: i16,
    tracks: i16,
    skid_sound: i16,
    _pad1: i16,
    x_drift: f32,
    y_drift: f32,
    x_front_drift: f32,
    y_front_drift: f32,
    track_slide: f32,
    dust_slide: f32,
    dust_color: u8,
    water: u8,
    _pad2: u16,
    slide_friction: f32,
};

pub fn load(allocator: Allocator, level_id: packs.Pack) !Level {
    // Ensure the level data is loaded in memory
    switch (level_id) {
        .level_01,
        .level_02,
        .level_03,
        => try packs.load(allocator, level_id),
        .level_04,
        .level_05,
        .level_06,
        .level_07,
        .level_08,
        .level_09,
        .level_10,
        => try packs.loadEncrypted(allocator, level_id, packs.decryption_key), // TODO: use key from registration
        else => return error.InvalidLevel,
    }

    const level = try packs.getEntry(Level, level_id, 1);
    const marks = try packs.getEntrySlice(Mark, allocator, level_id, 2);
    const road_info = try packs.getEntry(RoadInfo, .road, level.road_info_entry);

    _ = level;
    _ = marks;

    std.debug.print("{}\n", .{level});
    std.debug.print("{}\n", .{marks[0]});
    std.debug.print("{}\n", .{road_info});

    return level;
}
