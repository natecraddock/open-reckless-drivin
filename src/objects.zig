//! objects.zig

const packs = @import("packs.zig");
const random = @import("random.zig");
const std = @import("std");

const Allocator = std.mem.Allocator;
const ObjectTypeMap = std.AutoHashMap(i16, ObjectType);
const Point = @import("point.zig").Point;

// Information for a specific object
const Object = struct {
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

// Until packed bool structs can be cast to integer types reliably
// (allowing to be read from the Pack data), just use integer constants
// for the flag values

/// flags
const random_frame_flag: u16 = 1 << 3;
const heli_flag: u16 = 1 << 14;

/// flags2
const road_kill: u16 = 1 << 4;

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

    if (obtype.flags2 & road_kill != 0) {
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
