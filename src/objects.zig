//! objects.zig

const packs = @import("packs.zig");
const random = @import("random.zig");
const render = @import("render.zig");
const std = @import("std");
const trig = @import("trig.zig");
const utils = @import("utils.zig");

const Allocator = std.mem.Allocator;
const ArrayList = std.ArrayList;
const Game = @import("game.zig").Game;
const Level = @import("levels.zig").Level;
const ObjectTypeMap = std.AutoHashMap(i16, ObjectType);
const Point = @import("point.zig").Point;

const inf = std.math.inf_f32;
const max_rot_vel: f32 = 2 * trig.pi * 5;
const pixels_per_peter: f32 = 9.0;

// Information for a specific object
pub const Object = struct {
    pos: Point,
    velocity: Point,
    dir: f32,
    rot_vel: f32,
    slide: f32,
    throttle: f32,
    steering: f32,
    frame_duration: f32,
    jump_vel: f32,
    jump_height: f32,
    damage: f32,
    target: i32,
    control: ObjectControl,
    frame: i32,
    frame_repetition: i32,
    layer: i32,
    damage_flags: i32,
    type: *ObjectType,
    shooter: *anyopaque,
    // TODO: input tInputData,
};

const ObjectControl = enum(u8) {
    no_input,
    drive_up,
    drive_down,
    cross_road,
    cop_control,
};

/// Stores information about a type of object
const ObjectType = packed struct {
    mass: f32,
    max_engine_force: f32,
    max_neg_engine_force: f32,
    friction: f32,
    flags: u16,
    death_obj: i16,
    frame: i16,
    num_frames: u16,
    frame_duration: f32,
    wheel_width: f32,
    wheel_length: f32,
    steering: f32,
    width: f32,
    length: f32,
    score: u16,
    flags2: u16,
    creation_sound: i16,
    other_sound: i16,
    max_damage: f32,
    weapon_obj: i16,
    weapon_info: i16,
};

const ObjectGroup = packed struct {
    entry: i16,
    min_offs: i16,
    max_offs: i16,
    prob: i16,
    dir: f32,
};

pub const ObjectGroupRef = packed struct { id: i16, len: i16 };

// Until packed bool structs can be cast to integer types reliably
// (allowing to be read from the Pack data), just use integer constants
// for the flag values

/// flags
const wheel_flag: u16 = 1 << 0;
const solid_friction_flag: u16 = 1 << 1;
const back_collision_flag: u16 = 1 << 2;
const random_frame_flag: u16 = 1 << 3;
const die_when_anim_ends_flag: u16 = 1 << 4;
const default_death_flag: u16 = 1 << 5;
const follow_marks_flag: u16 = 1 << 6;
const overtake_flag: u16 = 1 << 7;
const slow_flag: u16 = 1 << 8;
const long_flag: u16 = 1 << 9;
const killed_by_cars_flag: u16 = 1 << 10;
const kills_cars_flag: u16 = 1 << 11;
const bounce_flag: u16 = 1 << 12;
const cop_flag: u16 = 1 << 13;
const heli_flag: u16 = 1 << 14;
const bonus_flag: u16 = 1 << 15;

/// flags2
const addon_flag: u16 = 1 << 0;
const front_collision_flag: u16 = 1 << 1;
const oil_flag: u16 = 1 << 2;
const missile_flag: u16 = 1 << 3;
const road_kill_flag: u16 = 1 << 4;
const layer_flag_1: u16 = 1 << 5;
const layer_flag_2: u16 = 1 << 6;
const engine_sound_flag: u16 = 1 << 7;
const ramp_flag: u16 = 1 << 8;
const sink_flag: u16 = 1 << 9;
const damageable_flag: u16 = 1 << 10;
const die_when_off_screen_flag: u16 = 1 << 11;
const rear_drive_flag: u16 = 1 << 12;
const rear_steer_flag: u16 = 1 << 13;
const object_floating_flag: u16 = 1 << 14;
const object_bump_flag: u16 = 1 << 15;

/// Due to endianness flipping, the object type data cannot be a simple pointer
/// into the pack data in the binary. Rather than load a copy for each object
/// type, each object type is loaded into this array and future lookups will
/// just reference this array to save memory.
/// Because object type data is sparse, it cannot easily be pre-loaded.
var object_types: ObjectTypeMap = undefined;

/// Initialize the object type table
pub fn initObjectTypes(allocator: Allocator) !void {
    object_types = ObjectTypeMap.init(allocator);
}

/// Free all memory used by the object type table
pub fn deinitObjectTypes() void {
    object_types.deinit();
}

/// Get a pointer to an object type. See comment above `object_types` for more information.
pub fn getObjectType(entry: i16) !*ObjectType {
    const gop = try object_types.getOrPut(entry);
    if (gop.found_existing) {
        return gop.value_ptr;
    } else {
        gop.value_ptr.* = try packs.getEntry(ObjectType, .object_type, entry);
        return gop.value_ptr;
    }
}

/// Create a new object of the given entry type
pub fn create(allocator: Allocator, entry: i16) !*Object {
    // TODO: is it important to zero the memory of this object?
    var object = try allocator.create(Object);
    var obtype = try getObjectType(entry);
    object.type = obtype;

    // TODO: layer flags
    object.layer = (obtype.flags2 >> 5) & 3;

    // Init specific object characteristics
    if (obtype.flags & random_frame_flag != 0) {
        object.frame = obtype.frame + random.randomInt(0, obtype.num_frames);
    } else {
        object.frame = obtype.frame;
    }

    if (obtype.flags2 & road_kill_flag != 0) {
        object.control = .cop_control;
    } else {
        object.control = .no_input;
    }

    if (obtype.flags & heli_flag != 0) {
        object.jump_height = 12.0;
    }

    if (obtype.creation_sound != 0) {
        // TODO: play sound
    }

    return object;
}

pub fn insertObjectGroup(allocator: Allocator, objects: *ArrayList(*Object), group: ObjectGroupRef) !void {
    var probabilities: [100]usize = undefined;
    // TODO: do I ever getEntryBytes without making a reader?
    const bytes = try packs.getEntryBytes(.object_groups, group.id);
    var reader = utils.Reader.init(bytes);
    const num_entries = try reader.read(u32);
    const groups = try reader.readSlice(ObjectGroup, allocator, num_entries);
    defer allocator.free(groups);

    // Fill probability table
    {
        var index: usize = 0;
        for (groups) |g, i| {
            var probCount: usize = 0;
            while (probCount < g.prob) : (probCount += 1) {
                probabilities[index] = i;
                index += 1;
            }
        }
    }

    var index: usize = 0;
    while (index < group.len) : (index += 1) {
        const prob = probabilities[@intCast(usize, random.randomInt(0, 100))];
        var object = try create(allocator, groups[prob].entry);
        object.pos = .{ .x = inf, .y = inf };
        object.dir = groups[prob].dir;
        var control: ObjectControl = undefined;
        object.pos = getUniquePosition(groups[prob].min_offs, groups[prob].max_offs, &object.dir, &control);
        object.control = control;

        // TODO: need to pass in level data
        if (object.control == .drive_up) {
            object.target = 0;
        } else {
            object.target = 0;
        }

        try objects.append(object);
    }
}

// TODO: need to pass in level data
pub fn getUniquePosition(min_offs: i16, max_offs: i16, object_dir: *f32, control: *ObjectControl) Point {
    _ = min_offs;
    _ = max_offs;
    _ = object_dir;
    _ = control;
    return .{ .x = inf, .y = inf };
}

/// Repair an object
inline fn repair(object: *Object) void {
    object.damage = 0;
    object.damage_flags = 0;
    // TODO: sprite unused
    object.frame = object.type.frame;
}

fn move(level: *Level, object: *Object) void {
    // Move objects
    if (object.velocity.x != 0.0 or object.velocity.y != 0.0) {
        object.pos = Point.add(object.pos, Point.scale(object.velocity, pixels_per_peter * render.frame_duration));

        // Cycle objects that moved off the track
        if (object.pos.y > @intToFloat(f32, level.road_data.len * 2)) {
            object.pos = .{
                .x = @intToFloat(f32, level.track_up[0].x),
                .y = 100,
            };
            if (object.control == .drive_up) object.target = 0;
            repair(object);
        } else if (object.pos.y < 0.0) {
            object.pos = .{
                .x = @intToFloat(f32, level.track_down[0].x),
                .y = @intToFloat(f32, level.road_data.len * 2 - 100),
            };
            if (object.control == .drive_down) object.target = 0;
            repair(object);
        }
    }

    // Handle water
    if (level.road_info.water != 0 and (object.type.flags2 & object_floating_flag != 0)) {
        object.pos = Point.add(object.pos, .{
            .x = -level.road_info.x_front_drift * 0.5 * render.frame_duration,
            .y = level.road_info.y_front_drift * 0.5 * render.frame_duration,
        });
    }

    // Rotations
    if (object.rot_vel != 0) {
        object.dir += object.rot_vel * render.frame_duration;

        if (object.dir >= 2 * trig.pi) {
            object.dir -= 2 * trig.pi;
        } else if (object.dir < 0.0) {
            object.dir += 2 * trig.pi;
        }

        // TODO: object.rot_vel = std.math.clamp(object.rot_vel, -max_rot_vel, max_rot_vel);
        if (std.math.fabs(object.rot_vel) > max_rot_vel) {
            object.rot_vel = if (object.rot_vel > 0) max_rot_vel else -max_rot_vel;
        }
    }

    // Handle jumping objects
    if (object.jump_vel != 0) {
        const gravity: f32 = 50.0;
        object.jump_height += object.jump_vel * render.frame_duration;
        object.jump_vel -= gravity * render.frame_duration;
        if (object.jump_vel == 0.0) object.jump_vel = -0.0001;

        // Place back on track?
        if (object.jump_height <= 0 and object.jump_vel <= 0) {
            object.jump_height = 0;
            object.jump_vel = 0;
            // TODO: sounds
        }
    }
}

/// Update sprite data for objects
fn animate(object: *Object) void {
    const obtype = object.type;
    if (obtype.frame_duration == 0) return;

    // Cops
    if (obtype.flags & cop_flag != 0 and obtype.flags & heli_flag == 0 and obtype.flags & engine_sound_flag == 0) {
        if (object.control != .cop_control) {
            object.frame = obtype.frame;
            return;
        }
    }

    object.frame_duration -= render.frame_duration;

    // Change sprite frames
    if (object.frame_duration <= 0.0) {
        object.frame_duration += obtype.frame_duration;
        if ((object.frame >= obtype.frame) and (object.frame < obtype.frame + @intCast(i16, obtype.num_frames & 0xff) - 1)) {
            object.frame += 1;
        } else if (obtype.flags & die_when_anim_ends_flag != 0 and (object.frame == obtype.frame + @intCast(i16, obtype.num_frames & 0xff) - 1)) {
            // TODO: kill object;
            return;
        } else {
            object.frame_duration += 1;
            object.frame = obtype.frame;
            // TODO: added a wrapping subtraction here, I think it's okay though
            if (object.frame_repetition == (obtype.num_frames >> 8) -% 1 or (obtype.num_frames >> 8 != 0)) {
                object.frame_repetition = 0;

                if (obtype.other_sound != 0) {
                    // TODO: play sounds
                }
            }
        }
    }

    // Don't animate dead roadkill?
    if (obtype.flags2 & road_kill_flag != 0 and (object.velocity.x == 0.0 and object.velocity.y == 0)) {
        object.frame = obtype.frame;
    }
}

/// Update all game objects
pub fn update(game: *Game, level: *Level) void {
    // TODO: special handling for the player
    // TODO: Read events with SDL
    for (level.objects.items) |object| {
        move(level, object);
        animate(object);
    }
    _ = game;
}
