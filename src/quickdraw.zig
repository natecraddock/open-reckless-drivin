//! quickdraw.zig
//! An implementation of a tiny subset of Macintosh QuickDraw 2D Graphics
//! See https://developer.apple.com/library/archive/documentation/mac/pdf/ImagingWithQuickDraw.pdf
//! for details. Appendix A is where the opcodes are documented.

const std = @import("std");

/// TODO: This will likely be pulled out to also be used for reading values
/// from sprites and other resources.
const Reader = struct {
    bytes: []const u8,
    index: usize,

    fn init(bytes: []const u8) Reader {
        return .{ .bytes = bytes, .index = 0 };
    }

    /// Read a number from the byte stream correcting for endianness
    fn readNumber(self: *Reader, comptime T: type) !T {
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

    /// Skip `count` bytes in the stream
    fn skip(self: *Reader, count: usize) !void {
        if (self.index + count > self.bytes.len) return error.EndOfStream;
        self.index += count;
    }
};

/// Bounds and the direct pixels representing a QuickDraw image.
///
/// Images 1000 through 1008 are stored in extended version 2 format and stored
/// compressed as PackBitsRects bitmaps. Image 1009 is version 2 and involves more
/// opcodes to render text on a bitmap.
///
/// Both versions have a header before the image data, but for Reckless Drivin'
/// the only useful information from the headers (after parsing) is the size of
/// the image. Everything else can be ignored.
const QuickDraw = struct {
    width: u16,
    height: u16,

    fn parse(bytes: []const u8) !QuickDraw {
        var reader = Reader.init(bytes);

        // Parse header
        // See listings A-5 and A-6 in Imaging With Quickdraw for reference
        try reader.skip(2); // picture size
        try reader.skip(4); // bounds top and left
        const height = try reader.readNumber(u16);
        const width = try reader.readNumber(u16);
        try reader.skip(6); // various version information
        const version = try reader.readNumber(u16);
        if (version == 0xFFFE) {
            try reader.skip(20);
        } else {
            try reader.skip(22);
        }

        // Parse remaining opcodes to construct the bitmap data
        while (reader.readNumber(u16)) |op| {
            switch (op) {
                0x0000, 0x0001 => continue, // NOP
                0x000A => {
                    // Region Size (skip)
                    try reader.skip(8);
                },
                0x0098 => {
                    // PackBitsRect
                    // TODO: get allocated pixels (or error) here
                },
                0x009A => {
                    // DirectBitsRect
                    // TODO: get allocated pixels (or error) here
                },
                0x00A1 => {
                    // LongComment (ignored)
                    try reader.skip(2);
                    const size = try reader.readNumber(u16);
                    try reader.skip(size);
                },
                0x00FF => break, // End Of Picture
                else => return error.UnknownOpcode,
            }
        } else |_| {
            // handle EOF when image not yet constructed
        }

        return QuickDraw{ .width = width, .height = height };
    }
};

const testing = std.testing;

test "quickdraw parsing" {
    const resources = @import("resources.zig");
    const lzrw = @import("lzrw.zig");

    const resource = resources.getResource("PPic", 1000) orelse return;
    const decompressed = try lzrw.decompressResource(testing.allocator, resource);

    const qd = QuickDraw.parse(decompressed);
    std.debug.print("{}\n", .{qd});

    // try expect(getResource("PPic", 1001) != null);
    // try expect(getResource("PPic", 1002) != null);
    // try expect(getResource("PPic", 1003) != null);
    // try expect(getResource("PPic", 1004) != null);
    // try expect(getResource("PPic", 1005) != null);
    // try expect(getResource("PPic", 1006) != null);
    // try expect(getResource("PPic", 1007) != null);
    // try expect(getResource("PPic", 1008) != null);
    // try expect(getResource("PPic", 1009) != null);
}
