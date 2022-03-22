const std = @import("std");
const log = std.log;

const version = "0.0";

pub fn main() void {
    log.info("Started Reckless Drivin {s}", .{version});
}

test {
    _ = @import("lzrw.zig");
    _ = @import("resources.zig");
    _ = @import("packs.zig");
}
