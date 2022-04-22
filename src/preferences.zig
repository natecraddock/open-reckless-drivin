//! preferences.zig
//! Preferences contains a basic INI parser, and the logic to parse
//! the Reckless Drivin' config format.

const std = @import("std");
const mem = std.mem;

const Allocator = mem.Allocator;

const INIParser = struct {
    iter: mem.TokenIterator(u8),
    section: ?[]const u8 = null,

    pub fn init(buf: []const u8) INIParser {
        return .{ .iter = mem.tokenize(u8, buf, "\n") };
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
        for (line) |c, i| {
            if (!std.ascii.isAlNum(c) and c != '_') {
                if (c == ']' and i == line.len - 1) break;
                return error.InvalidSyntax;
            }
        }
        return line[0 .. line.len - 1];
    }

    fn parse(line: []const u8) !Key {
        var iter = mem.tokenize(u8, line, "=");
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
            if (!std.ascii.isAlNum(c) and c != '_') return error.InvalidSyntax;
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

test "ini" {
    const s =
        \\#comment
        \\[sound]
        \\enabled = false
        \\volume = 100
        \\test="a long string here"
        \\[section1]
        \\[section2]
        \\sensitivity = 1.234
        \\[section3]
        \\name = value
    ;

    var parser = INIParser.init(s);
    while (true) {
        const keyOrNull = parser.next() catch continue;
        if (keyOrNull) |key| {
            std.debug.print("key: {s}.{s}={}\n", .{ parser.section, key.name, key.value });
        } else break;
    }
}
