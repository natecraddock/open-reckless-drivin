const std = @import("std");

const c = @cImport({
    @cInclude("raylib.h");
});



pub fn testing() void {
    c.InitWindow(640, 480, "Open Reckless Drivin'");
    c.SetTargetFPS(60);

    while (!c.WindowShouldClose()) {
        c.BeginDrawing();
        c.ClearBackground(c.RAYWHITE);
        c.DrawText("Congrats! You created your first window!", 190, 200, 20, c.LIGHTGRAY);
        c.EndDrawing();
    }

    c.CloseWindow();
}
