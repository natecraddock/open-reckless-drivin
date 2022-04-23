//! trig.zig: a global sine table
//! TODO: The original game used this. It is unclear at this point if the exact values
//! in this table would impact the gameplay, so to be safe we will include the table

const table_size: u16 = 1024;
const pi: f32 = 3.1415;

const sine_table = brk: {
    @setEvalBranchQuota(1025);
    var table: [table_size]f32 = undefined;
    var i: usize = 0;
    while (i < table_size) : (i += 1) {
        table[i] = @sin(2 * pi * (@intToFloat(f32, i) / @intToFloat(f32, table_size)));
    }
    break :brk table;
};

const sine_mask: u32 = 0x0000_03ff;

pub inline fn sin(x: f32) f32 {
    return sine_table[@floatToInt(u16, x * @intToFloat(f32, table_size) / (2.0 * pi)) & sine_mask];
}

pub inline fn cos(x: f32) f32 {
    return sine_table[@floatToInt(u16, (x + (pi / 2.0)) * @intToFloat(f32, table_size) / (2.0 * pi)) & sine_mask];
}

const testing = @import("std").testing;

test "trig functions" {
    // These tests make it clear that the sine table only stores approximations.
    // The true value of sin(6.89) is more precise than what is found in the table.
    // It remains to be seen if this is required for the game to function.
    try testing.expectApproxEqAbs(@as(f32, 0.0), sin(0), 0.01);
    try testing.expectApproxEqAbs(@as(f32, 1.0), sin(pi / 2.0), 0.01);
    try testing.expectApproxEqAbs(@as(f32, 0.57025), sin(6.89), 0.01);

    try testing.expectApproxEqAbs(@as(f32, 1.0), cos(0), 0.01);
    try testing.expectApproxEqAbs(@as(f32, 0.0), cos(pi / 2.0), 0.01);
    try testing.expectApproxEqAbs(@as(f32, 0.82146), cos(6.89), 0.01);
}
