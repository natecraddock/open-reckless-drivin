//! utils.zig: a place for shared functions and utility structs

const std = @import("std");

/// Read data from a stream of bytes
pub const Reader = struct {
    bytes: []const u8,
    index: usize,

    pub fn init(bytes: []const u8) Reader {
        return .{ .bytes = bytes, .index = 0 };
    }

    /// Read a number from the byte stream correcting for endianness
    pub fn readNumber(self: *Reader, comptime T: type) !T {
        const size = @sizeOf(T);
        // TODO: Because the data is constant, once the game works and
        // we confirm this is never reached we can remove the error check.
        if (self.index + size > self.bytes.len) return error.EndOfStream;

        if (size == 1) {
            const value = self.bytes[self.index];
            self.index += 1;
            return value;
        } else {
            const slice = self.bytes[self.index .. self.index + size];
            const value = std.mem.bigToNative(T, @ptrCast(*align(1) const T, slice).*);
            self.index += size;
            return value;
        }
    }

    /// Read bytes into an allocated slice
    pub fn readSlice(self: *Reader, comptime T: type, allocator: std.mem.Allocator, len: usize) ![]T {
        if (self.index + len > self.bytes.len) return error.EndOfStream;

        var slice = try allocator.alloc(T, len);
        for (slice) |*item| {
            item.* = try self.readNumber(T);
        }

        return slice;
    }

    /// Skip `count` bytes in the stream
    pub fn skip(self: *Reader, count: usize) !void {
        if (self.index + count > self.bytes.len) return error.EndOfStream;
        self.index += count;
    }
};
