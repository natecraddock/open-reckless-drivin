//! packs.zig
//! Most of the Resources in resources.zig are of type Pack. Packs are an
//! lzrw-compressed format used in the original Reckless Drivin' game, with
//! each pack containing multiple entities "packed" into a sequence of bytes.

const std = @import("std");

const Allocator = std.mem.Allocator;

const bigToNative = std.mem.bigToNative;
const nativeToBig = std.mem.nativeToBig;

const lzrw = @import("lzrw.zig");
const resources = @import("resources.zig");

// TODO: These names should match the descriptions in resources.zig
pub const Pack = enum(u8) {
    object_type,
    sprites,
    object_groups,
    rle,
    crle,
    textures,
    sounds,
    road,
    textures_16,
    sprites_16,
    rle_16,
    crle_16,
    level_01,
    level_02,
    level_03,
    level_04,
    level_05,
    level_06,
    level_07,
    level_08,
    level_09,
    level_10,
};

/// Level 4 marks the beginning of the encrypted packs
const encrypted_pack = Pack.level_04;

/// Packs are stored globally, and are read-only from other modules.
var packs = std.mem.zeroes([@typeInfo(Pack).Enum.fields.len]?[]const u8);

/// Load and decompress a Pack, preparing it for use later
pub fn load(allocator: Allocator, pack: Pack) !void {
    const id = @enumToInt(pack);
    if (packs[id] == null) {
        if (resources.getResource("Pack", id + 128)) |resource| {
            if (id >= @enumToInt(encrypted_pack)) {
                return error.BadDecompression;
            }
            packs[id] = try lzrw.decompressResource(allocator, resource);
        }
    }
}

/// Load, decrypt, and decompress a Pack, preparing it for use later
pub fn loadEncrypted(allocator: Allocator, pack: Pack, key: u32) !void {
    const id = @enumToInt(pack);
    if (packs[id] == null) {
        if (resources.getResource("Pack", id + 128)) |resource| {
            if (id < @enumToInt(encrypted_pack)) {
                return error.BadDecryption;
            }
            // Need working (non-const) memory to decrypt the pack in-place
            var buf = try allocator.alloc(u8, resource.len);
            defer allocator.free(buf);
            std.mem.copy(u8, buf, resource);
            _ = decrypt(buf, key);
            packs[id] = try lzrw.decompressResource(allocator, buf);
        }
    }
}

pub fn unload(allocator: Allocator, pack: Pack) void {
    const id = @enumToInt(pack);
    if (packs[id]) |p| {
        allocator.free(p);
        packs[id] = null;
    }
}

/// Each pack contains a list of headers (which may be sorted). Each header refers
/// to an offset to the byte where that entry begins in the pack bytes.
const Header = struct {
    entry: i16,
    pad: i16 = 0,
    offset: u32 = 0,

    fn compare(context: void, key: Header, item: Header) std.math.Order {
        _ = context;
        const entry = bigToNative(i16, item.entry);
        return std.math.order(key.entry, entry);
    }
};

/// Find an entry in a pack via binary search
/// TODO: could this potentially take a type parameter and parse the bytes
/// and return an instantiated struct instead? (one flaw, structs containing slices)
/// TODO: an additional feature would be a getEntry function that used a switch to
/// determine the search v normal version.
pub fn getEntrySparse(pack: Pack, entry: i16) ?[]const u8 {
    const id = @enumToInt(pack);
    if (packs[id] == null) return null;
    const bytes = @alignCast(@alignOf(Header), packs[id].?);

    const header = @ptrCast(*const Header, bytes[0..@sizeOf(Header)]);
    const num_entries = @intCast(usize, bigToNative(i16, header.entry));
    const entries = @ptrCast([*]const Header, bytes[@sizeOf(Header)..]);

    var key: Header = .{ .entry = entry };
    if (std.sort.binarySearch(Header, key, entries[0..num_entries], {}, Header.compare)) |index| {
        const offset = bigToNative(u32, entries[index].offset);
        var len: usize = undefined;
        if (index + 1 == num_entries) {
            len = bytes.len - offset;
        } else {
            len = bigToNative(u32, entries[index + 1].offset) - offset;
        }
        return bytes[offset .. offset + len];
    } else return null;
}

/// Decrypt the bytes with the given key. Returns a special check value
/// used to verify valid registration.
fn decrypt(bytes: []u8, key: u32) u32 {
    var check: u32 = 0;

    // The first 256 bytes of the pack are an unencrypted header
    var index: usize = 256;
    while (index <= bytes.len - 4) : (index += 4) {
        var word = @ptrCast(*align(1) u32, bytes[index .. index + 4]);
        word.* = bigToNative(u32, word.*) ^ key;
        check +%= word.*;
        word.* = nativeToBig(u32, word.*);
    }

    // Decrypt any bytes beyond a multiple of 4 bytes individually
    if (bytes.len -| index > 0) {
        bytes[index] ^= @intCast(u8, (key >> 24) & 0xff);
        check +%= bigToNative(u32, @intCast(u32, bytes[index]) << 24);
        index += 1;
    }
    if (bytes.len -| index > 0) {
        bytes[index] ^= @intCast(u8, (key >> 16) & 0xff);
        check +%= bigToNative(u32, @intCast(u32, bytes[index]) << 16);
        index += 1;
    }
    if (bytes.len -| index > 0) {
        bytes[index] ^= @intCast(u8, (key >> 8) & 0xff);
        check +%= bigToNative(u32, @intCast(u32, bytes[index]) << 8);
        index += 1;
    }

    return check;
}

const testing = std.testing;

// These tests for loading packs are considered to pass if
// there are no segmentation faults or memory leaks. Other tests
// will verify the integrity of the data.
test "pack loading" {
    try load(testing.allocator, .object_type);
    try load(testing.allocator, .sprites);
    try load(testing.allocator, .object_groups);
    try load(testing.allocator, .rle);
    try load(testing.allocator, .crle);
    try load(testing.allocator, .textures);
    try load(testing.allocator, .sounds);
    try load(testing.allocator, .road);
    try load(testing.allocator, .textures_16);
    try load(testing.allocator, .sprites_16);
    try load(testing.allocator, .rle_16);
    try load(testing.allocator, .crle_16);
    try load(testing.allocator, .level_01);
    try load(testing.allocator, .level_02);
    try load(testing.allocator, .level_03);

    // free all memory
    unload(testing.allocator, .object_type);
    unload(testing.allocator, .sprites);
    unload(testing.allocator, .object_groups);
    unload(testing.allocator, .rle);
    unload(testing.allocator, .crle);
    unload(testing.allocator, .textures);
    unload(testing.allocator, .sounds);
    unload(testing.allocator, .road);
    unload(testing.allocator, .textures_16);
    unload(testing.allocator, .sprites_16);
    unload(testing.allocator, .rle_16);
    unload(testing.allocator, .crle_16);
    unload(testing.allocator, .level_01);
    unload(testing.allocator, .level_02);
    unload(testing.allocator, .level_03);
}

test "pack decryption" {
    const key = 0x1E42A71F;

    try loadEncrypted(testing.allocator, .level_04, key);
    try loadEncrypted(testing.allocator, .level_05, key);
    try loadEncrypted(testing.allocator, .level_06, key);
    try loadEncrypted(testing.allocator, .level_07, key);
    try loadEncrypted(testing.allocator, .level_08, key);
    try loadEncrypted(testing.allocator, .level_09, key);
    try loadEncrypted(testing.allocator, .level_10, key);

    unload(testing.allocator, .level_04);
    unload(testing.allocator, .level_05);
    unload(testing.allocator, .level_06);
    unload(testing.allocator, .level_07);
    unload(testing.allocator, .level_08);
    unload(testing.allocator, .level_09);
    unload(testing.allocator, .level_10);
}

test "pack decryption check" {
    // The "Chck" resource is used to verify proper decryption of
    // level 04 by comparing the resource value to the value
    // computed during decryption of the level.
    const check = resources.getResource("Chck", 128) orelse return error.TestFailure;
    const check_val = bigToNative(u32, @ptrCast(*align(1) const u32, check).*);

    // Decrypt level 04 data to compute the decryption check
    const level_04 = resources.getResource("Pack", @enumToInt(Pack.level_04) + 128) orelse return error.TestFailure;
    var buf = try testing.allocator.alloc(u8, level_04.len);
    defer testing.allocator.free(buf);

    std.mem.copy(u8, buf, level_04);
    // TODO: store the global decryption key as a public constant somewhere
    const check_actual = decrypt(buf, 0x1E42A71F);

    try testing.expectEqual(check_val, check_actual);
}
