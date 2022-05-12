const std = @import("std");

const Sdk = @import("lib/sdl/Sdk.zig");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const exe = b.addExecutable("reckless-drivin", "src/main.zig");

    // Link SDL2
    const sdk = Sdk.init(b);
    sdk.link(exe, .dynamic);
    exe.addPackage(sdk.getWrapperPackage("sdl"));

    exe.linkLibC();
    exe.addIncludeDir("src/c/");
    exe.addCSourceFile("src/c/lzrw.c", &.{
        // The default is to enable undefined behavior detection in C code. I have
        // verified that the packs are all decompressed fine, so there is no need
        // to sanitize. See https://github.com/ziglang/zig/wiki/FAQ#why-do-i-get-illegal-instruction-when-using-with-zig-cc-to-build-c-code
        // for more details.
        "-fno-sanitize=undefined",
    });

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
    exe_tests.addCSourceFile("src/c/lzrw.c", &.{
        "-fno-sanitize=undefined",
    });

    exe_tests.setTarget(target);
    exe_tests.setBuildMode(mode);

    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&exe_tests.step);
}
