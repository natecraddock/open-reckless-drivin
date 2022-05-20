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

    /// Return a boolean indicating whether the flag is set
    pub fn flag(self: *ObjectType, f: ObjectTypeFlag) bool {
        // currently a packed bitflag struct does not have a defined integer
        // layout for deserialization. If that were the case I would use that
        // instead. This function is a workaround
        var value = @enumToInt(f);

        // This one function is used to manage both flag variables, so check
        // which flag this is intended for
        if (value < @enumToInt(ObjectTypeFlag.addon_flag)) {
            // flag1
            return self.flags & value != 0;
        } else {
            // flag2
            value -= 1 << 16;
            return self.flags2 & value != 0;
        }
    }
};

const ObjectGroup = packed struct {
    entry: i16,
    min_offs: i16,
    max_offs: i16,
    prob: i16,
    dir: f32,
};

pub const ObjectGroupRef = packed struct { id: i16, len: i16 };

/// the flags and flag2 fields are both stored in the enum.
pub const ObjectTypeFlag = enum(u32) {
    // flags
    wheel_flag = 1 << 0,
    solid_friction_flag = 1 << 1,
    back_collision_flag = 1 << 2,
    random_frame_flag = 1 << 3,
    die_when_anim_ends_flag = 1 << 4,
    default_death_flag = 1 << 5,
    follow_marks_flag = 1 << 6,
    overtake_flag = 1 << 7,
    slow_flag = 1 << 8,
    long_flag = 1 << 9,
    killed_by_cars_flag = 1 << 10,
    kills_cars_flag = 1 << 11,
    bounce_flag = 1 << 12,
    cop_flag = 1 << 13,
    heli_flag = 1 << 14,
    bonus_flag = 1 << 15,

    // flags2
    addon_flag = 1 << 16,
    front_collision_flag = 1 << 17,
    oil_flag = 1 << 18,
    missile_flag = 1 << 19,
    road_kill_flag = 1 << 20,
    layer_flag_1 = 1 << 21,
    layer_flag_2 = 1 << 22,
    engine_sound_flag = 1 << 23,
    ramp_flag = 1 << 24,
    sink_flag = 1 << 25,
    damageable_flag = 1 << 26,
    die_when_off_screen_flag = 1 << 27,
    rear_drive_flag = 1 << 28,
    rear_steer_flag = 1 << 29,
    object_floating_flag = 1 << 30,
    object_bump_flag = 1 << 31,
};

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
    if (obtype.flag(.random_frame_flag)) {
        object.frame = obtype.frame + random.randomInt(0, obtype.num_frames);
    } else {
        object.frame = obtype.frame;
    }

    if (obtype.flag(.road_kill_flag)) {
        object.control = .cop_control;
    } else {
        object.control = .no_input;
    }

    if (obtype.flag(.heli_flag)) {
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
    if (level.road_info.water != 0 and (object.type.flag(.object_floating_flag))) {
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

// TODO: honestly no idea what the one and two are intended to mean... but
// this is used in a few places. Will figure that out eventually and update
// the enums.
pub const Collision = enum { none, one, two };

fn calcBackCollision(level: *Level, pos: Point) Collision {
    var segments: [4]f32 = undefined;
    for (level.road_data[@floatToInt(usize, pos.y / 2.0)]) |segment, i| {
        segments[i] = @intToFloat(f32, segment);
    }
    const tolerance = @intToFloat(f32, level.road_info.tolerance);

    if (segments[0] > pos.x) {
        return if ((segments[0] - pos.x) > tolerance) .two else .one;
    }

    if (segments[3] < pos.x) {
        return if ((pos.x - segments[3]) > tolerance) .two else .one;
    }

    if ((segments[1] < pos.x) and (segments[2] > pos.x)) {
        const collision_val = if (pos.x - segments[1] < segments[2] - pos.x) pos.x - segments[1] else segments[2] - pos.x;
        return if (collision_val > 16) .two else .one;
    }

    return .none;
}

fn killObject(level: *Level, object: *Object) void {
    const obtype = object.type;
    const sink_enable = calcBackCollision(level, object.pos) == .two and (obtype.flag(.sink_flag));
    _ = sink_enable;
}

/// Update sprite data for objects
fn animate(level: *Level, object: *Object) void {
    const obtype = object.type;
    if (obtype.frame_duration == 0) return;

    // Cops
    if (obtype.flag(.cop_flag) and !obtype.flag(.heli_flag) and !obtype.flag(.engine_sound_flag)) {
        if (object.control != .cop_control) {
            object.frame = obtype.frame;
            return;
        }
    }

    object.frame_duration -= render.frame_duration;

    // Change sprite frames
    if (object.frame_duration <= 0.0) {
        object.frame_duration += obtype.frame_duration;
        if ((object.frame >= obtype.frame) and
            (object.frame < obtype.frame + @intCast(i16, obtype.num_frames & 0xff) - 1))
        {
            object.frame += 1;
        } else if (obtype.flag(.die_when_anim_ends_flag) and
            (object.frame == obtype.frame + @intCast(i16, obtype.num_frames & 0xff) - 1))
        {
            killObject(level, object);
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
    if (obtype.flag(.road_kill_flag) and (object.velocity.x == 0.0 and object.velocity.y == 0)) {
        object.frame = obtype.frame;
    }
}

/// Update all game objects
pub fn update(game: *Game, level: *Level) void {
    // TODO: special handling for the player
    // TODO: Read events with SDL
    for (level.objects.items) |object| {
        move(level, object);
        animate(level, object);
    }
    _ = game;
}
