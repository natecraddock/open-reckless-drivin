const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "reckless-drivin",
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibC();
    exe.addIncludePath("src/c/");
    exe.addCSourceFile("src/c/lzrw.c", &.{
        // The default is to enable undefined behavior detection in C code. I have
        // verified that the packs are all decompressed fine, so there is no need
        // to sanitize. See https://github.com/ziglang/zig/wiki/FAQ#why-do-i-get-illegal-instruction-when-using-with-zig-cc-to-build-c-code
        // for more details.
        "-fno-sanitize=undefined",
    });
    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run Reckless Drivin'");
    run_step.dependOn(&run_cmd.step);

    const exe_tests = b.addTest(.{
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });
    exe_tests.linkLibC();
    exe_tests.addIncludePath("src/c/");
    exe_tests.addCSourceFile("src/c/lzrw.c", &.{
        "-fno-sanitize=undefined",
    });

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&exe_tests.step);
}
