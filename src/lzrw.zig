//! lzrw.zig
//! A Zig interface to Ross Williams' LZRW3-A algorithm.

const std = @import("std");

const Allocator = std.mem.Allocator;

const c = @cImport({
    @cInclude("lzrw.h");
});

const pack_alignment = @import("packs.zig").pack_alignment;

fn decompress(allocator: Allocator, compressed: []const u8) ![]align(pack_alignment) u8 {
    // Create a buffer with enough space to store the decompressed bytes
    const identity = c.lzrw_identity();
    var working_mem = try allocator.alloc(u8, identity.memory);
    defer allocator.free(working_mem);

    // Maximum expansion possible is 9 times the length of the compressed data, use 10 for safety
    var destination_mem = try allocator.allocWithOptions(u8, compressed.len * 10, pack_alignment, null);

    var len: u64 = 0;
    c.lzrw3a_compress(
        c.COMPRESS_ACTION_DECOMPRESS,
        @ptrCast(working_mem),
        @ptrCast(compressed),
        @intCast(compressed.len),
        @ptrCast(destination_mem),
        &len,
    );

    destination_mem = try allocator.realloc(destination_mem, len);

    return destination_mem;
}

/// Decompress the given bytes using the LZRW3-A algorithm. The caller owns the
/// allocated slice of decompressed bytes.
pub fn decompressResource(allocator: Allocator, compressed: []const u8) ![]align(pack_alignment) u8 {
    // The offset of 4 was discovered by looking at the decompiled Reckless Drivin' source
    // in Ghidra. See https://nathancraddock.com/blog/resource-forks-and-lzrw-compression/
    // for more information.
    return try decompress(allocator, compressed[4..]);
}

const testing = std.testing;

// LZRW is only used for decompression, so there is no need to test compression
test "LZRW decompress" {
    {
        const result = try decompress(testing.allocator, "\x01\x00\x00\x00\x68\x65\x6c\x6c\x6f");
        defer testing.allocator.free(result);
        try testing.expectEqualSlices(u8, "hello", result);
    }

    {
        const result = try decompress(testing.allocator, "\x00\x00\x00\x00\x08\xfe\x61\x61\x61\x0f\x70\x61\x61\x61\x61\x61");
        defer testing.allocator.free(result);
        try testing.expectEqualSlices(u8, "aaaaaaaaaaaaaaaaaaaaaaaaaa", result);
    }

    {
        const result = try decompress(testing.allocator, "\x00\x00\x00\x00\x90\xff\x61\x73\x64\x66\x8f\x90\x64\x66");
        defer testing.allocator.free(result);
        try testing.expectEqualSlices(u8, "asdfasdfasdfasdfasdfasdf", result);
    }

    {
        const data = "\x00\x00\x00\x00\xf8\xff\x7a\x69\x67\x8f\xd0";
        const result = try decompress(testing.allocator, data);
        defer testing.allocator.free(result);
        try testing.expectEqualSlices(u8, "zigzigzigzigzigzigzig", result);
    }
}
