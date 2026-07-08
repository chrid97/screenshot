// ============================================================================
// TODO
// ============================================================================
//  - add capture modes to overlay
//  - use a preview buffer
//  - make overlay bigger?
//  - esc should cancel capture
//  - disable hotkeys when drawing
//  - when you're drawing under the buttons it should disappear or we should reduce the opacity
//  -undo/redo (maybe just undo)
// ============================================================================
// JUICE IDEAS
// ============================================================================
// - optional camera flash sound (maybe)
// - camera flash on screen?
// - mini image on the bottom right (configurable?)
// - animated squiggles
// ============================================================================
#include "capture.h"
#include "raylib.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// GLOBALS
// ============================================================================
Color CHARCOAL_OLIVE = { 26, 26, 20, 255 };
Color MIDNIGHT_BLUE = { 23, 56, 98, 255 };
Color ALABASTER = { 244, 241, 234, 255 };
Color HOTKEY = { 150, 145, 135, 255 }; // #969187
#define ACCENT_RED (Color){ 224, 49, 49, 255 }

Color stroke_color = ACCENT_RED;
// int stroke_width = 4;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

// ============================================================================
// TYPE DEFINITIONS
// ============================================================================
typedef enum { ACTION_RECTANGLE, ACTION_LINE, ACTION_FREEHAND, ACTION_CAPTURE } Draw;

typedef enum {
    OUTPUT_CLIPBOARD,
    OUTPUT_DISK,
} OutputDestination;

// ============================================================================
// CODE
// ============================================================================

bool inside_rounded_rect(int x, int y, int top, int left, int right, int bottom, int radius) {
    int cx = x;
    int cy = y;

    if (x < left + radius) {
        cx = left + radius;
    } else if (x >= right - radius) {
        cx = right - radius - 1;
    }

    if (y < top + radius) {
        cy = top + radius;
    } else if (y >= bottom - radius) {
        cy = bottom - radius - 1;
    }

    int dx = x - cx;
    int dy = y - cy;

    int distance_squared = dx * dx + dy * dy;
    return distance_squared <= radius * radius;
}

void draw_rect_rounded_outline(
    uint8_t *buffer, int initial_x, int initial_y, int curr_x, int curr_y, int image_width) {
    int radius = 10;
    int stroke_width = 3;
    int left = min_int(initial_x, curr_x);
    int right = max_int(initial_x, curr_x);
    int top = min_int(initial_y, curr_y);
    int bottom = max_int(initial_y, curr_y);
    for (int y = top; y <= bottom; y++) {
        for (int x = left; x <= right; x++) {
            bool outer = inside_rounded_rect(x, y, top, left, right, bottom, radius);
            bool inner = inside_rounded_rect(x,
                                             y,
                                             top + stroke_width,
                                             left + stroke_width,
                                             right - stroke_width,
                                             bottom - stroke_width,
                                             radius);

            if (outer && !inner) {
                int index = (y * image_width + x) * BYTES_PER_PIXEL;
                buffer[index + 0] = stroke_color.r;
                buffer[index + 1] = stroke_color.g;
                buffer[index + 2] = stroke_color.b;
                buffer[index + 3] = stroke_color.a;
            }
        }
    }
}

int main(void) {
    Draw action = ACTION_CAPTURE;
    Screenshot image = capture_screen();

    uint8_t *preview_buffer = malloc(screenshot_byte_count(&image));
    memcpy(preview_buffer, image.pixels, screenshot_byte_count(&image));

    // ============================================================================
    // Open raylib window
    // ============================================================================
    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);
    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_UNDECORATED);

    int monitor = GetCurrentMonitor();
    int w = GetMonitorWidth(monitor);
    int h = GetMonitorHeight(monitor);
    InitWindow(w, h, "Screenshot");

    Texture texture = LoadTextureFromImage((Image){
        .data = preview_buffer,
        .width = image.width,
        .height = image.height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
    });

    ToggleBorderlessWindowed();

    Vector2 initial_mouse_position = { 0 };
    Vector2 current_mouse_position = { 0 };
    Vector2 previous_mouse_position = { 0 };

    float scale_x = (float)image.width / (float)GetScreenWidth();
    float scale_y = (float)image.height / (float)GetScreenHeight();
    int stroke_width = (int)roundf(3.0f * scale_y);
    while (!WindowShouldClose()) {
        memcpy(preview_buffer, image.pixels, screenshot_byte_count(&image));

        // ============================================================================
        // Input Events
        // ============================================================================
        bool mouse_down = IsMouseButtonDown(MOUSE_LEFT_BUTTON);

        if (IsKeyPressed(KEY_ONE)) {
            action = ACTION_CAPTURE;
        }
        if (IsKeyPressed(KEY_TWO)) {
            action = ACTION_RECTANGLE;
        }
        if (IsKeyPressed(KEY_THREE)) {
            action = ACTION_LINE;
        }
        if (IsKeyPressed(KEY_FOUR)) {
            action = ACTION_FREEHAND;
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            // (CG) It might not be obvious why I set current_mouse_position here so maybe there's a
            // different way I can do this so its clear
            initial_mouse_position = GetMousePosition();
            current_mouse_position = GetMousePosition();
        }

        if (mouse_down) {
            previous_mouse_position = current_mouse_position;
            current_mouse_position = GetMousePosition();
        }

        int curr_x = (int)roundf(current_mouse_position.x * scale_x);
        int curr_y = (int)roundf(current_mouse_position.y * scale_y);
        int initial_x = (int)roundf(initial_mouse_position.x * scale_x);
        int initial_y = (int)roundf(initial_mouse_position.y * scale_y);

        // ============================================================================
        // Actions
        // ============================================================================

        if (mouse_down && action == ACTION_RECTANGLE) {
            draw_rect_rounded_outline(
                preview_buffer, initial_x, initial_y, curr_x, curr_y, image.width);
            UpdateTexture(texture, preview_buffer);
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            switch (action) {
            case ACTION_FREEHAND:
                break;
            case ACTION_RECTANGLE: {
                draw_rect_rounded_outline(
                    image.pixels, initial_x, initial_y, curr_x, curr_y, image.width);
                break;
            }
            case ACTION_LINE: {
                break;
            };
            case ACTION_CAPTURE: {
            }
            }
        }

        // ============================================================================
        //  Drawing
        // ============================================================================
        BeginDrawing();
        ClearBackground(BLACK);

        // Draw screenshot
        Rectangle src = { 0, 0, (float)texture.width, (float)texture.height };
        Rectangle dst = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
        DrawTexturePro(texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadTexture(texture);
    free(preview_buffer);
    free(image.pixels);
    CloseWindow();

    return 0;
}
