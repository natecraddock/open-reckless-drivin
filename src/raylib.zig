const std = @import("std");

pub const c = @cImport({
    @cInclude("raylib.h");
});

/// Initialize the window
pub fn initWindow(width: i32, height: i32, title: [:0]const u8) void {
    c.SetWindowState(c.FLAG_WINDOW_HIGHDPI);
    c.InitWindow(width, height, title);
    c.SetWindowState(c.FLAG_WINDOW_RESIZABLE | c.FLAG_VSYNC_HINT);
    c.SetExitKey(0);
    c.SetWindowMinSize(width, height);
    c.SetTargetFPS(60);
}

pub fn closeWindow() void {
    c.CloseWindow();
}

pub fn beginDrawing() void {
    c.BeginDrawing();
}

pub fn endDrawing() void {
    c.EndDrawing();
}

pub fn clearBackground() void {
    c.ClearBackground(c.BLACK);
}

pub fn windowShouldClose() bool {
    return c.WindowShouldClose();
}
