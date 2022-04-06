//! quickdraw.zig
//! An implementation of a tiny subset of Macintosh QuickDraw 2D Graphics
//! See https://developer.apple.com/library/archive/documentation/mac/pdf/ImagingWithQuickDraw.pdf
//! for details. Appendix A is where the opcodes are documented.

const std = @import("std");

const Allocator = std.mem.Allocator;

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
pub const QuickDraw = struct {
    width: u16,
    height: u16,
    pixels: []RGB = undefined,

    /// Parse a QuickDraw image from a given sequence of bytes
    pub fn parse(allocator: Allocator, bytes: []const u8) !QuickDraw {
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

        var image: QuickDraw = .{ .width = width, .height = height };

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
                    image.pixels = try parsePackBitsRect(allocator, &reader);
                },
                0x009A => {
                    // DirectBitsRect
                    image.pixels = try parseDirectBitsRect(allocator, &reader);
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

        return image;
    }
};

pub const RGB = struct { r: u16, g: u16, b: u16 };

fn parseDirectBitsRect(allocator: Allocator, reader: *Reader) ![]RGB {
    const map = try PixMap.parse(reader);
    try reader.skip(18); // srcRect, dstRect, mode

    const row_bytes = map.row_bytes & 0b0011_1111_1111_1111;

    const width = @intCast(u32, map.right - map.left);
    const num_pixels = @intCast(u32, map.bottom - map.top) * @intCast(u32, map.right - map.left);
    var pixels = try allocator.alloc(RGB, num_pixels);

    // Now read the actual pixel data
    const height = map.bottom - map.top;
    var line: usize = 0;
    while (line < height) : (line += 1) {
        if (row_bytes > 250) try reader.skip(2) else try reader.skip(1);
        try unpackRowDirect(reader, pixels[line * width .. line * width + width]);
    }

    return pixels;
}

fn parseColorTable(allocator: Allocator, reader: *Reader) ![]RGB {
    try reader.skip(6); // seed and flags
    // Size stores one less than the number of entries in the table
    const size = (try reader.readNumber(u16)) + 1;

    var table = try allocator.alloc(RGB, size);
    var i: usize = 0;
    while (i < size) : (i += 1) {
        try reader.skip(2); // index in table
        table[i] = .{
            .r = try reader.readNumber(u16),
            .g = try reader.readNumber(u16),
            .b = try reader.readNumber(u16),
        };
    }

    return table;
}

fn parsePackBitsRect(allocator: Allocator, reader: *Reader) ![]RGB {
    // Only PPic 1006 is a PackBitsRect image. For some reason the baseAddr
    // field of the inner PixMap is not present in the data, so we offset the
    // reader by -4 bytes. The baseAddr is ignored in the PixMap parsing so this
    // is fine.
    reader.index -= 4;
    const map = try PixMap.parse(reader);
    const color_table = try parseColorTable(allocator, reader);
    defer allocator.free(color_table);
    try reader.skip(18); // srcRect, dstRect, mode

    const row_bytes = map.row_bytes & 0b0011_1111_1111_1111;

    const width = @intCast(u32, map.right - map.left);
    const num_pixels = @intCast(u32, map.bottom - map.top) * @intCast(u32, map.right - map.left);
    var pixels = try allocator.alloc(RGB, num_pixels);

    const height = map.bottom - map.top;
    var line: usize = 0;
    while (line < height) : (line += 1) {
        if (row_bytes > 250) try reader.skip(2) else try reader.skip(1);
        try unpackRowPacked(reader, color_table, pixels[line * width .. line * width + width]);
    }

    return pixels;
}

/// Run-length decode a row of PackBits.
///
/// Although it seems like DirectBitsRect would be uncompressed, and PackBitsRect
/// would be compressed, PackBitsRect actually refers to indexing in a color table.
/// Both types of images can store linewise run-length encoded bitmap data. The
/// data is byte-oriented rather than pixel oriented. This information is missing
/// from Imaging with Quickdraw. See http://fileformats.archiveteam.org/wiki/PackBits
/// for more details.
fn unpackRowDirect(reader: *Reader, pixels: []RGB) !void {
    var col: usize = 0;
    while (col < pixels.len) {
        var n = try reader.readNumber(u8);
        if (n <= 127) {
            // Interpret the next n+1 bytes literally
            const repeat = n + 1;
            var i: usize = 0;
            while (i < repeat) : ({
                i += 1;
                col += 1;
            }) {
                const data = try reader.readNumber(u16);
                pixels[col].r = @intCast(u8, (data & 0b0111_1100_0000_0000) >> 10);
                pixels[col].g = @intCast(u8, (data & 0b0000_0011_1110_0000) >> 5);
                pixels[col].b = @intCast(u8, data & 0b0000_0000_0001_1111);
            }
        } else if (n == 128) {
            // Ignore
        } else {
            // Repeat the next byte 257 - n times
            const repeat = 257 - @intCast(u9, n);
            const data = try reader.readNumber(u16);
            const pixel: RGB = .{
                .r = @intCast(u8, (data & 0b0111_1100_0000_0000) >> 10),
                .g = @intCast(u8, (data & 0b0000_0011_1110_0000) >> 5),
                .b = @intCast(u8, data & 0b0000_0000_0001_1111),
            };
            var i: usize = 0;
            while (i < repeat) : ({
                i += 1;
                col += 1;
            }) {
                pixels[col] = pixel;
            }
        }
    }
}

/// The same as above, but for a PackedBitsRect.
/// The RLE data stores indexes into a color table rather than RGB values directly.
fn unpackRowPacked(reader: *Reader, color_table: []RGB, pixels: []RGB) !void {
    var col: usize = 0;
    while (col < pixels.len) {
        var n = try reader.readNumber(u8);
        if (n <= 127) {
            // Interpret the next n+1 bytes literally
            const repeat = n + 1;
            var i: usize = 0;
            while (i < repeat) : ({
                i += 1;
                col += 1;
            }) {
                const index = try reader.readNumber(u8);
                pixels[col] = color_table[index];
            }
        } else if (n == 128) {
            // Ignore
        } else {
            // Repeat the next byte 257 - n times
            const repeat = 257 - @intCast(u9, n);
            const index = try reader.readNumber(u8);
            const pixel = color_table[index];
            var i: usize = 0;
            while (i < repeat) : ({
                i += 1;
                col += 1;
            }) {
                pixels[col] = pixel;
            }
        }
    }
}

/// A pixel map contains information about the dimensions and contents
/// of a pixel image. See 4-46 in Imaging With Quickdraw for reference.
/// Reckless Drivin' only needs a subset of the full data.
const PixMap = struct {
    row_bytes: u16,
    pack_type: u16,
    pixel_type: u16,
    pixel_size: u16,
    top: u16,
    left: u16,
    bottom: u16,
    right: u16,

    fn parse(reader: *Reader) !PixMap {
        try reader.skip(4); // baseAddr
        const row_bytes = try reader.readNumber(u16);
        const top = try reader.readNumber(u16);
        const left = try reader.readNumber(u16);
        const bottom = try reader.readNumber(u16);
        const right = try reader.readNumber(u16);
        try reader.skip(2); // pmVersion
        const pack_type = try reader.readNumber(u16);
        try reader.skip(12); // packSize, hRes, and vRes
        const pixel_type = try reader.readNumber(u16);
        const pixel_size = try reader.readNumber(u16);
        try reader.skip(16); // cmpCount, cmpSize, planeBytes, and pmTable
        return PixMap{
            .row_bytes = row_bytes,
            .pack_type = pack_type,
            .pixel_type = pixel_type,
            .pixel_size = pixel_size,
            .top = top,
            .left = left,
            .bottom = bottom,
            .right = right,
        };
    }
};

const testing = std.testing;

test "quickdraw parsing" {
    const resources = @import("resources.zig");
    const lzrw = @import("lzrw.zig");

    // TODO: 1009 does not decompress
    const resource = resources.getResource("PPic", 1006) orelse return;
    const decompressed = try lzrw.decompressResource(testing.allocator, resource);
    defer testing.allocator.free(decompressed);

    const qd = try QuickDraw.parse(testing.allocator, decompressed);
    defer testing.allocator.free(qd.pixels);

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
