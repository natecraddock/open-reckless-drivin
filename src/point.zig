//!: point.zig: types and functions for working with point data

pub const Point = extern struct {
    x: f32 align(1),
    y: f32 align(1),

    pub fn add(a: Point, b: Point) Point {
        return .{ .x = a.x + b.x, .y = a.y + b.y };
    }

    pub fn diff(a: Point, b: Point) Point {
        return .{ .x = a.x - b.x, .y = a.y - b.y };
    }

    pub fn scale(p: Point, scalar: f32) Point {
        return .{ .x = p.x * scalar, .y = p.y * scalar };
    }

    pub fn value(p: Point) f32 {
        return @sqrt(p.x * p.x + p.y * p.y);
    }
};
