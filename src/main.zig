//! main.zig: handles argument parsing for the game and other utitlities

const std = @import("std");
const log = std.log;
const mem = std.mem;

const Allocator = std.mem.Allocator;

const game = @import("game.zig");
const lzrw = @import("lzrw.zig");
const packs = @import("packs.zig");
const resources = @import("resources.zig");

const version = "0.0";

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    const allocator = gpa.allocator();
    defer _ = gpa.deinit();

    const args = try std.process.argsAlloc(allocator);
    defer std.process.argsFree(allocator, args);
    if (args.len > 1) {
        try handleArgs(allocator, args);
    } else {
        log.info("Started Reckless Drivin {s}", .{version});
        try game.start(allocator);
    }
}

fn handleArgs(allocator: Allocator, args: [][:0]u8) !void {
    if (mem.eql(u8, args[1], "register")) {
        try registerNewUser(allocator, args);
    } else if (mem.eql(u8, args[1], "dump-resource")) {
        try dumpResource(allocator, args);
    }
}

/// Register a new player for Reckless Drivin'
/// Given a name generates a 10 charater registration code.
fn registerNewUser(allocator: Allocator, args: [][:0]u8) !void {
    const stdout = std.io.getStdOut().writer();
    const stderr = std.io.getStdErr().writer();
    if (args.len != 3) {
        try stderr.print("usage: {s} register [name]\n", .{args[0]});
        return;
    }

    const name = mem.trim(u8, args[2], " ");
    if (name.len < 4) {
        try stderr.print("name must be at least 4 characters long\n", .{});
        return;
    }

    const name_formatted = try formatName(allocator, name);
    defer allocator.free(name_formatted);

    // This is the decryption key that will be regenerated through a
    // combination of the registered player's name and code.
    const key = 0x1e42a71f;

    const name_num = mem.nativeToBig(u32, @ptrCast(*align(1) u32, name_formatted[name_formatted.len - 4 ..]).*);
    const code_num = key ^ name_num;

    // This seed is used to scramble the code_num. I cannot determine how
    // it was generated in the original Reckless Drivin' (there are an ininite
    // number of possibilities) so just sum all the letters of the name mod 0xFF
    // to generate something unique per registration code.
    var seed: u32 = 0;
    for (name_formatted) |n| seed += n;
    seed %= 0xFF;

    const seed_mask = seed + (seed << 8) + (seed << 16) + (seed << 24);
    const code_first_8 = code_num ^ seed_mask;

    try stdout.print("Successfully Registered!\nname: {s}\ncode: {X:08}{X:02}\n", .{ name, code_first_8, seed });
}

/// Remove spaces and uppercase the name
fn formatName(allocator: Allocator, name: []const u8) ![]u8 {
    var buf = try allocator.alloc(u8, name.len);
    var i: usize = 0;
    for (name) |n| {
        if (n == ' ') continue;
        buf[i] = std.ascii.toUpper(n);
        i += 1;
    }

    // There is a small chance that the buffer contains less than 4 chars,
    // so pad with the letter 'A' if needed
    while (i < 4) : (i += 1) buf[i] = 'A';

    buf = try allocator.realloc(buf, i);
    return buf;
}

fn dumpResource(allocator: Allocator, args: [][:0]const u8) !void {
    const stdout = std.io.getStdOut().writer();
    const stderr = std.io.getStdErr().writer();
    if (args.len != 4) {
        try stderr.print("usage: {s} {s} [type] [id]\n", .{ args[0], args[1] });
        return;
    }

    const resource_type = std.mem.trim(u8, args[2], " ");
    const id = std.fmt.parseInt(u16, std.mem.trim(u8, args[3], " "), 0) catch {
        try stderr.print("error: invalid character or overflow for resource id\n", .{});
        return;
    };

    if (resources.getResource(resource_type, id)) |resource| {
        if (mem.eql(u8, resource_type, "Chck")) {
            _ = try stdout.write(resource);
        } else if (mem.eql(u8, resource_type, "Pack") and id > 142) {
            var buf = try allocator.dupe(u8, resource);
            defer allocator.free(buf);
            _ = packs.decrypt(buf, packs.decryption_key);
            const decompressed = try lzrw.decompressResource(allocator, buf);
            defer allocator.free(decompressed);
            _ = try stdout.write(decompressed);
        } else {
            const decompressed = try lzrw.decompressResource(allocator, resource);
            defer allocator.free(decompressed);
            _ = try stdout.write(decompressed);
        }
    } else {
        try stderr.print("error: invalid resource\n", .{});
    }
}

comptime {
    _ = @import("lzrw.zig");
    _ = @import("objects.zig");
    _ = @import("packs.zig");
    _ = @import("preferences.zig");
    _ = @import("quickdraw.zig");
    _ = @import("random.zig");
    _ = @import("resources.zig");
    _ = @import("sprites.zig");
    _ = @import("trig.zig");
}
