//! level.zig: for managing the data for each game level

const objects = @import("objects.zig");
const packs = @import("packs.zig");
const std = @import("std");
const utils = @import("utils.zig");

const Allocator = std.mem.Allocator;
const ObjectGroupRef = objects.ObjectGroupRef;
const Point = @import("point.zig").Point;

const Level = packed struct {
    road_info_entry: i16,
    time: u16,
    object_groups: [10]ObjectGroupRef,
    x_start: i16,
    level_end: u16,
};

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

const TrackInfo = packed struct {
    flags: u16,
    x: i16,
    y: i32,
    velocity: f32,
};

const ObjectPosition = packed struct {
    x: i32,
    y: i32,
    dir: f32,
    entry: i16,
    _pad: i16,
};

const RoadSegment = packed struct { parts: [4]i16 };

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

    // The first entry in a level (the level header info) contains many bytes after
    // the header before the next entry. These must be parsed manually to get more
    // information about the road.
    var reader = utils.Reader.init(try packs.getEntryBytes(level_id, 1));
    try reader.skip(@sizeOf(Level));

    // I believe these are the positions and velocities of the objects on the track
    // those travelling up, and those travelling down.
    const track_up = try readTrackInfo(allocator, &reader);
    const track_down = try readTrackInfo(allocator, &reader);

    // Read object positions and create objects
    const num_obs = try reader.read(u32);
    const object_positions = try reader.readSlice(ObjectPosition, allocator, num_obs);
    defer allocator.free(object_positions);
    for (object_positions) |pos| {
        // TODO: leak! Store these objects somewhere
        var object = try objects.create(allocator, pos.entry);
        object.dir = pos.dir;
        object.pos.x = @intToFloat(f32, pos.x);
        object.pos.y = @intToFloat(f32, pos.y);
    }

    // Read road data
    const road_length = try reader.read(u32);
    const road_data = try reader.readSlice(RoadSegment, allocator, road_length);

    // Create object groups
    for (level.object_groups[0 .. level.object_groups.len - 1]) |group| {
        if (group.id != 0) try objects.insertObjectGroup(allocator, group);
    }

    // Create player object

    std.debug.print("{}\n", .{level});
    std.debug.print("{}\n", .{marks[0]});
    std.debug.print("{}\n", .{road_info});
    std.debug.print("{}\n", .{track_up[0]});
    std.debug.print("{}\n", .{track_down[0]});
    std.debug.print("{}\n", .{road_length});
    std.debug.print("{}\n", .{road_data[0]});

    return level;
}

fn readTrackInfo(allocator: Allocator, reader: *utils.Reader) ![]const TrackInfo {
    const len = try reader.read(u32);
    return try reader.readSlice(TrackInfo, allocator, len);
}
