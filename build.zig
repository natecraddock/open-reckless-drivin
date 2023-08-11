const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const raylib = b.dependency("raylib", .{
        .target = target,
        .optimize = optimize,
    });

    const exe = b.addExecutable(.{
        .name = "reckless-drivin",
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });

    exe.linkLibC();
    exe.linkLibrary(raylib.artifact("raylib"));

    exe.addIncludePath(.{ .path = "src/c/" });
    exe.addCSourceFile(.{
        .file = .{ .path = "src/c/lzrw.c" },
        .flags = &.{
            // The default is to enable undefined behavior detection in C code. I have
            // verified that the packs are all decompressed fine, so there is no need
            // to sanitize. See https://github.com/ziglang/zig/wiki/FAQ#why-do-i-get-illegal-instruction-when-using-with-zig-cc-to-build-c-code
            // for more details.
            "-fno-sanitize=undefined",
        },
    });

    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| run_cmd.addArgs(args);

    const run_step = b.step("run", "Run Reckless Drivin'");
    run_step.dependOn(&run_cmd.step);

    const tests = b.addTest(.{
        .root_source_file = .{ .path = "src/main.zig" },
        .target = target,
        .optimize = optimize,
    });
    tests.linkLibC();
    tests.addIncludePath(.{ .path = "src/c/" });
    tests.addCSourceFile(.{
        .file = .{ .path = "src/c/lzrw.c" },
        .flags = &.{"-fno-sanitize=undefined"},
    });

    const run_tests = b.addRunArtifact(tests);
    const test_step = b.step("test", "Run tests");
    test_step.dependOn(&run_tests.step);
}
