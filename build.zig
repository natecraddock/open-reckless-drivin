const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("reckless-drivin", "src/main.zig");

    exe.linkLibC();
    exe.addIncludeDir("src/c/");
    exe.addCSourceFile("src/c/lzrw.c", &.{});

    exe.setTarget(target);
    exe.setBuildMode(mode);
    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run Reckless Drivin'");
    run_step.dependOn(&run_cmd.step);

    const exe_tests = b.addTest("src/main.zig");
    exe_tests.linkLibC();
    exe_tests.addIncludeDir("src/c/");
    exe_tests.addCSourceFile("src/c/lzrw.c", &.{});

    exe_tests.setTarget(target);
    exe_tests.setBuildMode(mode);

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&exe_tests.step);
}
