//! utils.zig: a place for shared functions and utility structs

const std = @import("std");

const bigToNative = std.mem.bigToNative;

/// Read data from a stream of bytes
pub const Reader = struct {
    bytes: []const u8,
    index: usize,

    pub fn init(bytes: []const u8) Reader {
        return .{ .bytes = bytes, .index = 0 };
    }

    /// Read an integer from the byte stream correcting for endianness
    fn readInt(self: *Reader, comptime T: type) !T {
        const size = @sizeOf(T);
        // TODO: Because the data is constant, once the game works and
        // we confirm this is never reached we can remove the error check.
        if (self.index + size > self.bytes.len) return error.EndOfStream;

        const slice = self.bytes[self.index .. self.index + size];
        const value = @ptrCast(*align(1) const T, slice).*;

        self.index += size;
        return bigToNative(T, value);
    }

    /// Read a float from the byte stream correcting for endianness
    fn readFloat(self: *Reader) !f32 {
        const size = @sizeOf(f32);
        if (self.index + size > self.bytes.len) return error.EndOfStream;

        const slice = self.bytes[self.index .. self.index + size];
        const value = @ptrCast(*align(1) const u32, slice).*;

        self.index += size;
        // The float needs to be interpreted as an integer to flip the endianness
        return @bitCast(f32, bigToNative(u32, value));
    }

    /// Read bytes into an allocated slice
    pub fn readSlice(self: *Reader, comptime T: type, allocator: std.mem.Allocator, len: usize) ![]T {
        if (self.index + len > self.bytes.len) return error.EndOfStream;

        var slice = try allocator.alloc(T, len);
        for (slice) |*item| {
            item.* = try self.read(T);
        }

        return slice;
    }

    /// Read bytes into a struct. The struct type must be packed to guarantee order of fields
    /// Limitations:
    /// * does not support arbitrary slices of bytes
    fn readStruct(self: *Reader, comptime T: type) !T {
        const fields = std.meta.fields(T);

        var item: T = undefined;
        inline for (fields) |field| {
            @field(item, field.name) = try self.read(field.field_type);
        }

        return item;
    }

    /// Read a value of the specified type from the byte stream
    /// Types will be corrected for endianness if the host is not big-endian
    pub fn read(self: *Reader, comptime T: type) !T {
        return switch (@typeInfo(T)) {
            .Int => try self.readInt(T),
            .Float => try self.readFloat(),
            .Array => |array| {
                var arr: [array.len]array.child = undefined;
                var index: usize = 0;
                while (index < array.len) : (index += 1) {
                    arr[index] = try self.read(array.child);
                }
                return arr;
            },
            .Struct => try self.readStruct(T),
            else => @compileError("unsupported type"),
        };
    }

    /// Skip `count` bytes in the stream
    pub fn skip(self: *Reader, count: usize) !void {
        if (self.index + count > self.bytes.len) return error.EndOfStream;
        self.index += count;
    }
};
