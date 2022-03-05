//! resources.zig
//! Resources are the data from the resource fork in the original Macintosh game.
//! The data file is embedded directly in the binary.

const std = @import("std");
const eql = std.mem.eql;

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
const data = @embedFile("resources.dat");

const Header = packed struct {
    resource_type: [8]u8,
    id: u32,
    length: u32,
};

const Resource = struct {
    header: Header,
    data: []const u8,
};

fn resourceIter(offset: *usize) ?Resource {
    if (offset.* >= data.len) return null;

    const header = @ptrCast(*const Header, data[offset.* .. offset.* + @sizeOf(Header)]);
    offset.* += @sizeOf(Header);

    const resource: Resource = .{
        .header = header.*,
        .data = data[offset.* .. offset.* + header.length],
    };

    offset.* += header.length;
    return resource;
}

/// Search through the resource data for the resource of the given type and id
/// Returns a slice of bytes or null if not found
fn findResource(resource_type: []const u8, id: u16) ?[]const u8 {
    var offset: usize = 0;
    while (resourceIter(&offset)) |resource| {
        if (eql(u8, resource.header.resource_type[0..4], resource_type) and resource.header.id == id) {
            return resource.data;
        }
    }
    return null;
}

const expect = std.testing.expect;

test "findResource" {
    // Ensure all valid resources can be found
    try expect(findResource("Pack", 128) != null);
    try expect(findResource("Pack", 129) != null);
    try expect(findResource("Pack", 130) != null);
    try expect(findResource("Pack", 131) != null);
    try expect(findResource("Pack", 132) != null);
    try expect(findResource("Pack", 133) != null);
    try expect(findResource("Pack", 134) != null);
    try expect(findResource("Pack", 135) != null);
    try expect(findResource("Pack", 136) != null);
    try expect(findResource("Pack", 137) != null);
    try expect(findResource("Pack", 138) != null);
    try expect(findResource("Pack", 139) != null);
    try expect(findResource("Pack", 140) != null);
    try expect(findResource("Pack", 141) != null);
    try expect(findResource("Pack", 142) != null);
    try expect(findResource("Pack", 143) != null);
    try expect(findResource("Pack", 144) != null);
    try expect(findResource("Pack", 145) != null);
    try expect(findResource("Pack", 146) != null);
    try expect(findResource("Pack", 147) != null);
    try expect(findResource("Pack", 148) != null);
    try expect(findResource("Pack", 149) != null);
    try expect(findResource("PPic", 1000) != null);
    try expect(findResource("PPic", 1001) != null);
    try expect(findResource("PPic", 1002) != null);
    try expect(findResource("PPic", 1003) != null);
    try expect(findResource("PPic", 1004) != null);
    try expect(findResource("PPic", 1005) != null);
    try expect(findResource("PPic", 1006) != null);
    try expect(findResource("PPic", 1007) != null);
    try expect(findResource("PPic", 1008) != null);
    try expect(findResource("PPic", 1009) != null);
    try expect(findResource("Chck", 128) != null);

    // Test a few invalid resources
    try expect(findResource("Pack", 127) == null);
    try expect(findResource("Pack", 150) == null);
    try expect(findResource("PPic", 999) == null);
    try expect(findResource("PPic", 1010) == null);
    try expect(findResource("Chck", 129) == null);
    try expect(findResource("None", 128) == null);
}

pub fn getResource(resource_type: []const u8, id: u16) ?[]const u8 {
    _ = resource_type;
    _ = id;
}
