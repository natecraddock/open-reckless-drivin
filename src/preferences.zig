//! preferences.zig
//! Preferences contains a basic INI parser, and the logic to parse
//! the Reckless Drivin' config format.

const std = @import("std");
const mem = std.mem;

const Allocator = mem.Allocator;

/// TODO:
/// * trailing line comments
const INIParser = struct {
    iter: mem.TokenIterator(u8, .scalar),
    section: ?[]const u8 = null,

    pub fn init(buf: []const u8) INIParser {
        return .{ .iter = mem.tokenizeScalar(u8, buf, '\n') };
    }

    pub fn next(self: *INIParser) !?Key {
        while (self.iter.next()) |line| {
            const line_trimmed = mem.trim(u8, line, " ");

            // Ignore whitespace and comments
            if (line_trimmed.len == 0 or line_trimmed[0] == '#') continue;

            // Parse section headers
            if (line_trimmed[0] == '[') {
                self.section = try parseSection(line_trimmed[1..]);
                continue;
            }

            // Parse keys
            return try parse(line_trimmed);
        } else return null;
    }

    /// A valid section contains at least 1 character surrounded by square brackets.
    /// The inside characters must not be square brackets.
    fn parseSection(line: []const u8) ![]const u8 {
        if (line.len == 1) return error.InvalidSyntax;
        for (line, 0..) |c, i| {
            if (!std.ascii.isAlphanumeric(c) and c != '_') {
                if (c == ']' and i == line.len - 1) return line[0 .. line.len - 1];
                return error.InvalidSyntax;
            }
        }
        return error.InvalidSyntax;
    }

    fn parse(line: []const u8) !Key {
        var iter = mem.tokenizeScalar(u8, line, '=');
        const name = iter.next();
        if (name == null) {
            return error.InvalidSyntax;
        }
        const value = iter.next();
        if (value == null) {
            return error.InvalidSyntax;
        }

        return Key{
            .name = try parseName(mem.trim(u8, name.?, " ")),
            .value = try parseValue(mem.trim(u8, value.?, " ")),
        };
    }

    fn parseName(name: []const u8) ![]const u8 {
        for (name) |c| {
            if (!std.ascii.isAlphanumeric(c) and c != '_') return error.InvalidSyntax;
        }
        return name;
    }

    fn parseValue(value: []const u8) !Value {
        if (mem.eql(u8, value, "true")) {
            return Value{ .bool = true };
        } else if (mem.eql(u8, value, "false")) {
            return Value{ .bool = false };
        } else if (value[0] == '"' and value[value.len - 1] == '"') {
            return Value{ .str = value[1 .. value.len - 1] };
        } else {
            if (std.fmt.parseInt(i32, value, 0)) |int| {
                return Value{ .int = int };
            } else |_| {
                if (std.fmt.parseFloat(f32, value)) |float| {
                    return Value{ .float = float };
                } else |_| {
                    return error.InvalidSyntax;
                }
            }
        }
    }
};

const Key = struct {
    name: []const u8,
    value: Value,
};

const Value = union(enum) {
    str: []const u8,
    int: i32,
    float: f32,
    bool: bool,
};

const testing = std.testing;
const expectError = testing.expectError;
const expectString = testing.expectEqualStrings;

test "ini" {
    const s =
        \\# comment (should be ignored)
        \\     #  leading spaces are ignored
        \\a_bool = true
        \\# sections
        \\[sound]
        \\enabled = false
        \\volume = 100
        \\
        \\[another_section]
        \\test="a long string here"
        \\empty=""
        \\sensitivity    =     1.234
        \\
        \\# now for some errors
        \\[error
        \\=
        \\a=
        \\=a
        \\val=t
        \\val=truee
        \\val="some unclosed string
    ;

    var parser = INIParser.init(s);

    // valid cases
    try expectEqualKey("a_bool", Value{ .bool = true }, (try parser.next()).?);
    try testing.expectEqual(@as(?[]const u8, null), parser.section);
    try expectEqualKey("enabled", Value{ .bool = false }, (try parser.next()).?);
    try testing.expectEqualStrings("sound", parser.section.?);
    try expectEqualKey("volume", Value{ .int = 100 }, (try parser.next()).?);
    try expectEqualKey("test", Value{ .str = "a long string here" }, (try parser.next()).?);
    try testing.expectEqualStrings("another_section", parser.section.?);
    try expectEqualKey("empty", Value{ .str = "" }, (try parser.next()).?);
    try expectEqualKey("sensitivity", Value{ .float = 1.234 }, (try parser.next()).?);

    // error cases
    try expectError(error.InvalidSyntax, parser.next());
    try expectError(error.InvalidSyntax, parser.next());
    try expectError(error.InvalidSyntax, parser.next());
    try expectError(error.InvalidSyntax, parser.next());
    try expectError(error.InvalidSyntax, parser.next());
    try expectError(error.InvalidSyntax, parser.next());
    try expectError(error.InvalidSyntax, parser.next());

    // end of file
    try testing.expectEqual(@as(?Key, null), try parser.next());
}

fn expectEqualKey(name: []const u8, value: Value, actual: Key) !void {
    try testing.expectEqualStrings(name, actual.name);
    switch (value) {
        .str => try testing.expectEqualStrings(value.str, actual.value.str),
        else => try testing.expectEqual(value, actual.value),
    }
}
