const std = @import("std");
const log = std.log;

const version = "0.0";

pub fn main() void {
    log.info("Started Reckless Drivin {s}", .{version});
}

comptime {
    _ = @import("lzrw.zig");
    _ = @import("packs.zig");
    _ = @import("quickdraw.zig");
    _ = @import("resources.zig");
}
