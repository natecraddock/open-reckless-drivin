//! utils.zig: a place for shared functions and utility structs

const std = @import("std");

/// Read data from a stream of bytes
pub const Reader = struct {
    bytes: []const u8,
    index: usize,

    pub fn init(bytes: []const u8) Reader {
        return .{ .bytes = bytes, .index = 0 };
    }

    /// Read an integer from the byte stream correcting for endianness
    pub fn readInt(self: *Reader, comptime T: type) !T {
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

    /// Read a float from the byte stream correcting for endianness
    pub fn readFloat(self: *Reader, comptime T: type) !f32 {
        // Reckless Drivin' only ever uses f32 values in structs that need parsing.
        // It is therefore okay to only accept f32 types.
        if (T != f32) @compileError("readFloat() only reads f32 values");

        const size = @sizeOf(T);
        const slice = self.bytes[self.index .. self.index + size];
        // The value needs to be read as an integer to flip the endianness
        const value = std.mem.bigToNative(u32, @ptrCast(*align(1) const u32, slice).*);
        self.index += size;
        return @bitCast(T, value);
    }

    /// Read bytes into an allocated slice
    pub fn readSlice(self: *Reader, comptime T: type, allocator: std.mem.Allocator, len: usize) ![]T {
        if (self.index + len > self.bytes.len) return error.EndOfStream;

        var slice = try allocator.alloc(T, len);
        for (slice) |*item| {
            item.* = try self.readInt(T);
        }

        return slice;
    }

    /// Read bytes into a struct. The struct type must be packed to guarantee order of fields
    /// Limitations:
    /// * does not support arbitrary slices of bytes
    pub fn readStruct(self: *Reader, comptime T: type) !T {
        const fields = switch (@typeInfo(T).Struct.layout) {
            .Packed => std.meta.fields(T),
            else => @compileError("readStruct() must use a packed struct type"),
        };

        var item: T = undefined;
        inline for (fields) |field| {
            switch (@typeInfo(field.field_type)) {
                .Int => @field(item, field.name) = try self.readInt(field.field_type),
                .Float => @field(item, field.name) = try self.readFloat(field.field_type),
                else => @compileError("unsupported type '" ++ @typeName(field.field_type) ++ "'"),
            }
        }

        return item;
    }

    /// Skip `count` bytes in the stream
    pub fn skip(self: *Reader, count: usize) !void {
        if (self.index + count > self.bytes.len) return error.EndOfStream;
        self.index += count;
    }
};
