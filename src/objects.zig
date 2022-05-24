//! objects.zig

const packs = @import("packs.zig");
const random = @import("random.zig");
const render = @import("render.zig");
const sprites = @import("sprites.zig");
const std = @import("std");
const trig = @import("trig.zig");
const utils = @import("utils.zig");

const Allocator = std.mem.Allocator;
const Game = @import("game.zig").Game;
const Level = @import("levels.zig").Level;
const ObjectTypeMap = std.AutoHashMap(i16, ObjectType);
const Point = @import("point.zig").Point;

const inf = std.math.inf_f32;
const max_rot_vel: f32 = 2 * trig.pi * 5;
const pixels_per_meter: f32 = 9.0;

/// Data for an individual object
pub const ObjectData = struct {
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
    // TODO: perhaps store a pointer to the player data so all that is easily accessible?
    is_player: bool, // Reckless Drivin' uses a global, but a boolean is much simpler even if it is nearly always false
    // TODO: input tInputData,
};

/// A list of objects
pub const ObjectList = std.TailQueue(ObjectData);

/// An object node with pointer to previous and next objects
pub const Object = ObjectList.Node;

const ObjectControl = enum(u8) {
    no_input,
    drive_up,
    drive_down,
    cross_road,
    cop_control,
};

const ObjectGroup = packed struct {
    entry: i16,
    min_offs: i16,
    max_offs: i16,
    prob: i16,
    dir: f32,
};

pub const ObjectGroupRef = packed struct { id: i16, len: i16 };

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
            value >>= 16;
            return self.flags2 & value != 0;
        }
    }
};

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

test "object type flags" {
    // Construct some obtype flags on boundaries
    var obtype: ObjectType = undefined;
    obtype.flags = @enumToInt(ObjectTypeFlag.wheel_flag) + @enumToInt(ObjectTypeFlag.bonus_flag);

    // Note that this is a mess, but we shouldn't ever need to construct
    // ourselves in game code
    obtype.flags2 = @intCast(u16, (@enumToInt(ObjectTypeFlag.addon_flag) >> 16) + (@enumToInt(ObjectTypeFlag.object_bump_flag) >> 16));

    try std.testing.expect(obtype.flag(.wheel_flag));
    try std.testing.expect(obtype.flag(.bonus_flag));
    try std.testing.expect(obtype.flag(.addon_flag));
    try std.testing.expect(obtype.flag(.object_bump_flag));

    try std.testing.expect(!obtype.flag(.slow_flag));
    try std.testing.expect(!obtype.flag(.sink_flag));
}

/// Create a new object of the given entry type
pub fn create(allocator: Allocator, entry: i16) !*ObjectList.Node {
    // TODO: is it important to zero the memory of this object?
    var object = try allocator.create(ObjectList.Node);
    var obdata = &object.data;

    var obtype = try getObjectType(entry);
    obdata.type = obtype;

    // TODO: layer flags
    obdata.layer = (obtype.flags2 >> 5) & 3;

    // Init specific object characteristics
    if (obtype.flag(.random_frame_flag)) {
        obdata.frame = obtype.frame + random.randomInt(0, obtype.num_frames);
    } else {
        obdata.frame = obtype.frame;
    }

    if (obtype.flag(.road_kill_flag)) {
        obdata.control = .cop_control;
    } else {
        obdata.control = .no_input;
    }

    if (obtype.flag(.heli_flag)) {
        obdata.jump_height = 12.0;
    }

    if (obtype.creation_sound != 0) {
        // TODO: play sound
    }

    obdata.is_player = false;

    return object;
}

pub fn insertObjectGroup(allocator: Allocator, objects: *ObjectList, group: ObjectGroupRef) !void {
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
        var obdata = &object.data;

        obdata.pos = .{ .x = inf, .y = inf };
        obdata.dir = groups[prob].dir;
        var control: ObjectControl = undefined;
        obdata.pos = getUniquePosition(groups[prob].min_offs, groups[prob].max_offs, &obdata.dir, &control);
        obdata.control = control;

        // TODO: need to pass in level data
        if (obdata.control == .drive_up) {
            obdata.target = 0;
        } else {
            obdata.target = 0;
        }

        objects.append(object);
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
inline fn repair(game: *Game, obdata: *ObjectData) void {
    obdata.damage = 0;
    obdata.damage_flags = 0;
    game.specialSpriteUnused(obdata.frame);
    obdata.frame = obdata.type.frame;
}

fn move(game: *Game, level: *Level, object: *Object) void {
    var obdata = &object.data;

    // Move objects
    if (obdata.velocity.x != 0.0 or obdata.velocity.y != 0.0) {
        obdata.pos = Point.add(obdata.pos, Point.scale(obdata.velocity, pixels_per_meter * render.frame_duration));

        // Cycle objects that moved off the track
        if (obdata.pos.y > @intToFloat(f32, level.road_data.len * 2)) {
            obdata.pos = .{
                .x = @intToFloat(f32, level.track_up[0].x),
                .y = 100,
            };
            if (obdata.control == .drive_up) obdata.target = 0;
            repair(game, obdata);
        } else if (obdata.pos.y < 0.0) {
            obdata.pos = .{
                .x = @intToFloat(f32, level.track_down[0].x),
                .y = @intToFloat(f32, level.road_data.len * 2 - 100),
            };
            if (obdata.control == .drive_down) obdata.target = 0;
            repair(game, obdata);
        }
    }

    // Handle water
    if (level.road_info.water != 0 and (obdata.type.flag(.object_floating_flag))) {
        obdata.pos = Point.add(obdata.pos, .{
            .x = -level.road_info.x_front_drift * 0.5 * render.frame_duration,
            .y = level.road_info.y_front_drift * 0.5 * render.frame_duration,
        });
    }

    // Rotations
    if (obdata.rot_vel != 0) {
        obdata.dir += obdata.rot_vel * render.frame_duration;

        if (obdata.dir >= 2 * trig.pi) {
            obdata.dir -= 2 * trig.pi;
        } else if (obdata.dir < 0.0) {
            obdata.dir += 2 * trig.pi;
        }

        // TODO: obdata.rot_vel = std.math.clamp(obdata.rot_vel, -max_rot_vel, max_rot_vel);
        if (std.math.fabs(obdata.rot_vel) > max_rot_vel) {
            obdata.rot_vel = if (obdata.rot_vel > 0) max_rot_vel else -max_rot_vel;
        }
    }

    // Handle jumping objects
    if (obdata.jump_vel != 0) {
        const gravity: f32 = 50.0;
        obdata.jump_height += obdata.jump_vel * render.frame_duration;
        obdata.jump_vel -= gravity * render.frame_duration;
        if (obdata.jump_vel == 0.0) obdata.jump_vel = -0.0001;

        // Place back on track?
        if (obdata.jump_height <= 0 and obdata.jump_vel <= 0) {
            obdata.jump_height = 0;
            obdata.jump_vel = 0;
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

fn killObject(game: *Game, level: *Level, object: *Object) void {
    var obdata = &object.data;
    var obtype = obdata.type;

    if (obdata.is_player) {
        // TODO: show killed player text effect
        // TODO: player addons and other stats

        obdata.slide = 0;
        obdata.throttle = 0;
    } else if (obtype.score != 0) {
        // TODO: show score text effect
    }

    game.specialSpriteUnused(obdata.frame);
    if (obtype.flag(.default_death_flag)) {
        // TODO: Explosion!
    }

    if (obtype.death_obj == -1) {
        level.removeObject(game.allocator, object);
        return;
    }

    const sink_enable = calcBackCollision(level, obdata.pos) == .two and (obtype.flag(.sink_flag));
    const death_obj = obtype.death_obj + if (sink_enable) level.road_info.death_offs else 0;
    obdata.type = getObjectType(death_obj) catch unreachable;

    obdata.layer = obtype.flags2 >> 5 & 3;
    obtype = obdata.type;

    if (obtype.flag(.random_frame_flag)) {
        obdata.frame = obtype.frame + random.randomInt(0, obtype.num_frames);
    } else {
        obdata.frame = obtype.frame;
    }

    if (obtype.creation_sound != 0) {
        // TODO: play sounds
    }

    obdata.frame_duration = obtype.frame_duration;
    obdata.control = .no_input;
}

/// Update sprite data for objects
fn animate(game: *Game, level: *Level, object: *Object) void {
    var obdata = &object.data;
    const obtype = obdata.type;

    if (obtype.frame_duration == 0) return;

    // Cops
    if (obtype.flag(.cop_flag) and !obtype.flag(.heli_flag) and !obtype.flag(.engine_sound_flag)) {
        if (obdata.control != .cop_control) {
            obdata.frame = obtype.frame;
            return;
        }
    }

    obdata.frame_duration -= render.frame_duration;

    // Change sprite frames
    if (obdata.frame_duration <= 0.0) {
        obdata.frame_duration += obtype.frame_duration;
        if ((obdata.frame >= obtype.frame) and
            (obdata.frame < obtype.frame + @intCast(i16, obtype.num_frames & 0xff) - 1))
        {
            obdata.frame += 1;
        } else if (obtype.flag(.die_when_anim_ends_flag) and
            (obdata.frame == obtype.frame + @intCast(i16, obtype.num_frames & 0xff) - 1))
        {
            killObject(game, level, object);
            return;
        } else {
            obdata.frame_duration += 1;
            obdata.frame = obtype.frame;
            // TODO: added a wrapping subtraction here, I think it's okay though
            if (obdata.frame_repetition == (obtype.num_frames >> 8) -% 1 or (obtype.num_frames >> 8 != 0)) {
                obdata.frame_repetition = 0;

                if (obtype.other_sound != 0) {
                    // TODO: play sounds
                }
            }
        }
    }

    // Don't animate dead roadkill?
    if (obtype.flag(.road_kill_flag) and (obdata.velocity.x == 0.0 and obdata.velocity.y == 0)) {
        obdata.frame = obtype.frame;
    }
}

/// Update all game objects
pub fn update(game: *Game, level: *Level) void {
    // TODO: special handling for the player
    // TODO: Read events with SDL
    var it = level.objects.first;
    while (it) |object| : (it = object.next) {
        move(game, level, object);
        animate(game, level, object);
    }
    _ = game;
}
