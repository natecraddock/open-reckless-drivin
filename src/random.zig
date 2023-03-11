//! random.zig: a simple psuedorandom number generator
//! The original source used this random number generator written by
//! M. Matsumoto. See the original source for more details. This is a
//! version translated to Zig.
//!
//! It isn't clear if this specific algorithm is needed, so for now we
//! will use it and later see if it can be replaced with a Zig PRNG.

const std = @import("std");
const math = std.math;

const Prng = std.rand.DefaultPrng;

/// Constants for random number generation
const N: u32 = 25;
const M: u32 = 7;
const magic = [2]u64{ 0x0, 0x8ebf_d028 };

/// Initial random seeds for the random number generator
var seeds: [N]u64 = undefined;

/// Generate the next random number in the sequence using the algorithm from M. Matsumoto.
var k: i32 = 0;
fn nextRandom() f64 {
    if (k == N) {
        var kk: u32 = 0;
        while (kk < N - M) : (kk += 1) {
            seeds[kk] = seeds[kk + M] ^ (seeds[kk] >> 1) ^ magic[seeds[kk] % 2];
        }
        while (kk < N) : (kk += 1) {
            seeds[kk] = seeds[kk - 18] ^ (seeds[kk] >> 1) ^ magic[seeds[kk] % 2];
        }
        k = 0;
    }

    var y = seeds[@intCast(usize, k)];
    y ^= (y << 7) & 0x2b5b_2500; // s and b, magic vectors
    y ^= (y << 15) & 0xdb8b_0000; // t and c, magic vectors
    y &= 0xffffffff;

    y ^= (y >> 16);
    k += 1;

    return @intToFloat(f64, y) / @intToFloat(f64, 0xffff_ffff);
}

/// Initialize the random number seeds
pub fn init(initial_seed: u64) void {
    var prng = Prng.init(initial_seed);
    var rand = prng.random();

    for (seeds) |*seed| {
        seed.* = rand.int(u64);
    }
}

/// Return a random float evenly distributed in the interval [min, max]
pub fn randomFloat(min: f32, max: f32) f32 {
    return nextRandom() * (max - min) + min;
}

/// Return a random int evenly distributed in the interval [min, max-1]
pub fn randomInt(min: i32, max: i32) i32 {
    const min_f = @intToFloat(f64, min);
    const max_f = @intToFloat(f64, max);

    var r = nextRandom();
    while (r == 1) r = nextRandom();
    return @floatToInt(i32, r * (max_f - min_f) + min_f);
}

/// Returns true with a probability of p
/// (i.e. if p is is 1 always return true, if p is 0 always return false)
pub fn randomProb(p: f32) bool {
    return nextRandom() <= p;
}

const expect = std.testing.expect;

test "random" {
    // init with a deterministic seed
    init(2);

    // expect these values
    try expect(randomInt(0, 100) == 80);
    try expect(randomInt(0, 100) == 45);
    try expect(randomInt(0, 100) == 86);
    try expect(randomInt(0, 100) == 79);
    try expect(randomInt(0, 100) == 41);
    try expect(randomInt(0, 100) == 84);
    try expect(randomInt(0, 100) == 65);
    try expect(randomInt(0, 100) == 29);
    try expect(randomInt(0, 100) == 49);
    try expect(randomInt(0, 100) == 62);
}
