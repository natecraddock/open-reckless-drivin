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
    allocator.free(packs[id].?);
    packs[id] = null;
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
