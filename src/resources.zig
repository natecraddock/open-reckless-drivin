//! resources.zig
//! Resources are the data from the resource fork in the original Macintosh game.
//! The data file is embedded directly in the binary.

const std = @import("std");

const Reader = @import("utils.zig").Reader;

const eql = std.mem.eql;
const bigToNative = std.mem.bigToNative;

/// resources.dat contains the resource fork data. Each resource begins with a
/// Header containing the type, id, and resource length in bytes, followed by
/// the bytes for that resource. Resource data is constant during runtime.
///
/// List of resources:
/// * Pack 128 Object Definitions
/// * Pack 129 Sprites
/// * Pack 130 Object Group Definitions
/// * Pack 131 Panel Graphics
/// * Pack 132 Game Font
/// * Pack 133 Textures
/// * Pack 134 Sounds
/// * Pack 135 Road Characteristics
/// * Pack 136 16-Bit Textures
/// * Pack 137 16-Bit Sprites
/// * Pack 138 16-Bit Panel Graphics
/// * Pack 139 16-Bit Game Font
/// * Pack 140 Level 1
/// * Pack 141 Level 2
/// * Pack 142 Level 3
/// * Pack 143 Level 4
/// * Pack 144 Level 5
/// * Pack 145 Level 6
/// * Pack 146 Level 7
/// * Pack 147 Level 8
/// * Pack 148 Level 9
/// * Pack 149 Level 10
/// * PPic 1000 Main Menu
/// * PPic 1001 Main Menu Hover
/// * PPic 1002 Main Menu Click
/// * PPic 1003 Loading
/// * PPic 1004 High Scores
/// * PPic 1005 End of Unregistered Play
/// * PPic 1006 Paused
/// * PPic 1007 Help 1
/// * PPic 1008 Help 2
/// * PPic 1009 Credits
/// * Chck 128 Decryption Validation Bytes
/// * Cl16 8 16-bit color lookup table
///
/// A note about data alignment: The bytes in the resource file are just bytes,
/// so there isn't any reason to align the data to a word boundary. This will
/// take place later when splitting into individual resources
const data = @embedFile("resources.dat");

const Header = extern struct {
    resource_type: [8]u8 align(1),
    id: u32 align(1),
    length: u32 align(1),
};

const Resource = struct {
    header: Header,
    data: []const u8,
};

fn resourceIter(reader: *Reader) ?Resource {
    var header = reader.read(Header) catch return null;

    // All other uses of the reader are parsing big-endian data. Here we need to
    // manually flip the bytes. This is an acceptable trade off rather than making
    // the Reader more complex for only this use case
    header.id = bigToNative(u32, header.id);
    header.length = bigToNative(u32, header.length);

    const resource: Resource = .{
        .header = header,
        .data = data[reader.index .. reader.index + header.length],
    };

    reader.skip(header.length) catch return null;
    return resource;
}

/// Search through the resource data for the resource of the given type and id
/// Returns a slice of bytes or null if not found
pub fn getResource(resource_type: []const u8, id: u16) ?[]const u8 {
    var reader = Reader.init(data);
    while (resourceIter(&reader)) |resource| {
        if (eql(u8, resource.header.resource_type[0..4], resource_type) and resource.header.id == id) {
            return resource.data;
        }
    }
    return null;
}

// TODO: perhaps add a getPicResource() fn to make it easier to get and decompress
// an lzrw image? It will be used in a few places in the code so a function makes sense
// perhaps here or in packs.zig

const expect = std.testing.expect;

test "getResource" {
    // Ensure all valid resources can be found
    try expect(getResource("Pack", 128) != null);
    try expect(getResource("Pack", 129) != null);
    try expect(getResource("Pack", 130) != null);
    try expect(getResource("Pack", 131) != null);
    try expect(getResource("Pack", 132) != null);
    try expect(getResource("Pack", 133) != null);
    try expect(getResource("Pack", 134) != null);
    try expect(getResource("Pack", 135) != null);
    try expect(getResource("Pack", 136) != null);
    try expect(getResource("Pack", 137) != null);
    try expect(getResource("Pack", 138) != null);
    try expect(getResource("Pack", 139) != null);
    try expect(getResource("Pack", 140) != null);
    try expect(getResource("Pack", 141) != null);
    try expect(getResource("Pack", 142) != null);
    try expect(getResource("Pack", 143) != null);
    try expect(getResource("Pack", 144) != null);
    try expect(getResource("Pack", 145) != null);
    try expect(getResource("Pack", 146) != null);
    try expect(getResource("Pack", 147) != null);
    try expect(getResource("Pack", 148) != null);
    try expect(getResource("Pack", 149) != null);
    try expect(getResource("PPic", 1000) != null);
    try expect(getResource("PPic", 1001) != null);
    try expect(getResource("PPic", 1002) != null);
    try expect(getResource("PPic", 1003) != null);
    try expect(getResource("PPic", 1004) != null);
    try expect(getResource("PPic", 1005) != null);
    try expect(getResource("PPic", 1006) != null);
    try expect(getResource("PPic", 1007) != null);
    try expect(getResource("PPic", 1008) != null);
    try expect(getResource("PPic", 1009) != null);
    try expect(getResource("Chck", 128) != null);
    try expect(getResource("Cl16", 8) != null);

    // Test a few invalid resources
    try expect(getResource("Pack", 127) == null);
    try expect(getResource("Pack", 150) == null);
    try expect(getResource("PPic", 999) == null);
    try expect(getResource("PPic", 1010) == null);
    try expect(getResource("Chck", 129) == null);
    try expect(getResource("None", 128) == null);
}
