const g = @import("g.zig");
const random = @import("random.zig");
const render = @import("render.zig");
const std = @import("std");
const window = @import("window.zig");

const Point = @import("point.zig").Point;

const e = 2.71828182846;
const max_particles = 64;
const max_emitters = 64;
const particle_duration = 0.5;

/// A particle emitter
///
/// tParticleFx
const Emitter = struct {
    active: bool,
    num_particles: i32,

    /// Layer the particle emitter is on
    top: bool,

    start_frame: u32,
    // color: u16,
    pos: Point,

    particles: [max_particles * 4]Point = undefined,
};

/// All active particle emitters
///
/// gParticleFX
var emitters: [max_emitters]Emitter = undefined;

pub fn new(pos: Point, v: Point, num: i32, color: u8, top: bool, spread: f32) void {
    _ = color;
    // Find an empty slot
    var slot: u32 = 0;
    while (slot < max_emitters and emitters[slot].active) : (slot += 1) {}
    if (slot >= max_emitters) return;

    emitters[slot] = Emitter{
        .active = true,
        .top = top,
        .num_particles = num,
        .start_frame = g.frame_count,
        .pos = pos,
        // TODO: figure out the color look up table
        // .color = clut[color],
    };

    const num_particles = @min(max_particles, num * 4);
    for (0..num_particles) |i| {
        emitters[slot].particles[i] = v.add(.{ .x = random.randomFloat(-spread, spread), .y = random.randomFloat(-spread, spread) });
    }
}

pub fn draw(pixels: []u16, x_draw: f32, y_draw: f32, zoom: f32, top: bool) void {
    _ = pixels;
    const inv_zoom = 1.0 / zoom;

    var slot: u32 = 0;
    while (slot < max_emitters) : (slot += 1) {
        if (!emitters[slot].active or emitters[slot].top != top) continue;

        const emitter = &emitters[slot];
        const dt = @as(f32, @floatFromInt(g.frame_count - emitter.start_frame)) * render.frame_duration;

        // Changed to u32 from i32
        const num_draw_particles = @min(
            4 * max_particles,
            @as(u32, @intFromFloat(@as(f32, @floatFromInt(emitter.num_particles)) * inv_zoom * inv_zoom)),
        );

        if (dt >= particle_duration) {
            emitter.active = false;
            continue;
        }

        for (0..num_draw_particles) |i| {
            const epdt = -std.math.pow(f32, e, -dt);
            const pos = emitter.pos.add(.{
                .x = emitter.particles[i].x * epdt + emitter.particles[i].x,
                .y = emitter.particles[i].y * epdt + emitter.particles[i].y,
            });

            const x = (pos.x - x_draw) * inv_zoom;
            const y = (y_draw - pos.y) * inv_zoom;
            if (x > 0 and y > 0 and x < window.width and y < window.height) {
                // pixels[y * window.row_bytes + x * 2] = emitter.color;
            }
        }
    }
}
