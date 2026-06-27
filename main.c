#include "capture.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// (CG) Might wanna split out action capture, maybe make a tagged union
typedef enum { ACTION_RECTANGLE, ACTION_LINE, ACTION_FREEHAND, ACTION_CAPTURE } Draw;

typedef enum {
    CAPTURE_MODE_SCREEN,
    CAPTURE_MODE_REGION,
    CAPTURE_MODE_WINDOW,
} CaptureMode;

typedef enum {
    OUTPUT_CLIPBOARD,
    OUTPUT_DISK,
} OutputDestination;

int main(void) {
    CaptureMode capture_mode = CAPTURE_MODE_REGION;
    Draw action = ACTION_FREEHAND;
    size_t size = 0;

    // ============================================================================
    // Capture Screenshot
    // ============================================================================
    Screenshot screenshot = { 0 };
#if defined(__linux__)
    double t_capture_start = now_ms();
    FILE *file = popen("grim -t ppm -", "r");
    char magic[3];
    int max_ppm_value = 0;
    fscanf(file, "%2s", magic);
    // check if ppm image p6
    fscanf(file, "%d %d", &screenshot.width, &screenshot.height);
    fscanf(file, "%d", &max_ppm_value);
    fgetc(file); // consume newline after 255

    // why use size_t not int?
    size = screenshot.width * screenshot.height * 3;
    screenshot.pixels = (uint8_t *)malloc(size);
    fread(screenshot.pixels, 1, size, file);
    pclose(file);

    double t_capture_end = now_ms();
    printf("capture: %.2f ms\n", t_capture_end - t_capture_start);
#elif defined(__APPLE__)
    screenshot = capture_screen();
#elif defined(_WIN32)
    screenshot = capture_screen();
#else
#error Unsupported platform
#endif

    int pixel_format = 0;
    int bytes_per_pixel = 0;
#if defined(__linux__)
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    bytes_per_pixel = 3;
#elif defined(__APPLE__)
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    bytes_per_pixel = 4;
#elif defined(_WIN32)
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    bytes_per_pixel = 4;
#endif
    double t_window_start = now_ms();
    Image original_image = {
        .data = screenshot.pixels,
        .width = screenshot.width,
        .height = screenshot.height,
        .mipmaps = 1,
        .format = pixel_format,
    };

    Image image = original_image;

    size = screenshot.height * screenshot.width * bytes_per_pixel;
    image.data = (uint8_t *)malloc(size);
    memcpy(image.data, original_image.data, size);

    SetTraceLogLevel(LOG_ERROR);
    SetTargetFPS(60);

    // ============================================================================
    // Open raylib window
    // ============================================================================
    double t = now_ms();
    int monitor = GetCurrentMonitor();
    int w = GetMonitorWidth(monitor);
    int h = GetMonitorHeight(monitor);

    SetConfigFlags(FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_UNDECORATED);
    InitWindow(w, h, "Screenshot");
    printf("InitWindow: %.2f ms\n", now_ms() - t);

    t = now_ms();
    ToggleBorderlessWindowed();
    printf("ToggleBorderlessWindowed: %.2f ms\n", now_ms() - t);

    double t_window_end = now_ms();
    printf("window creation: %.2f ms\n", t_window_end - t_window_start);

    Texture2D texture = LoadTextureFromImage(image);
    printf("screen: %d x %d\n", GetScreenWidth(), GetScreenHeight());
    printf("render: %d x %d\n", GetRenderWidth(), GetRenderHeight());

    Vector2 initial_mouse_position = { 0 };
    Vector2 current_mouse_position = { 0 };

    uint8_t *pixels = (uint8_t *)image.data;
    while (!WindowShouldClose()) {
        // initial_mouse_position = (Vector2){ 0 };
        current_mouse_position = (Vector2){ 0 };

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
            initial_mouse_position = GetMousePosition();
            printf("%f, %f\n", GetMousePosition().x, GetMousePosition().y);
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            current_mouse_position = GetMousePosition();

            switch (action) {
            case ACTION_CAPTURE: {
                break;
            }
            case ACTION_FREEHAND: {
                float scale_x = (float)image.width / (float)GetScreenWidth();
                float scale_y = (float)image.height / (float)GetScreenHeight();
                int y = scale_y * current_mouse_position.y;
                int x = scale_x * current_mouse_position.x;

                int index = (y * image.width + x) * bytes_per_pixel;
                pixels[index] = 255;
                pixels[index + 1] = 255;
                pixels[index + 2] = 255;
                if (bytes_per_pixel == 4) {
                    pixels[index + 3] = 255;
                }
                UpdateTexture(texture, image.data);
                break;
            };
            case ACTION_LINE: {
                break;
            }
            case ACTION_RECTANGLE: {
                break;
            }
            }
        }

        if (action == ACTION_CAPTURE && IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            printf("On release %f, %f\n", current_mouse_position.x, current_mouse_position.y);

            // ON macos screenshots are in retina space
            // mouse is in screen space
            // we want to scale mosue coords up to retina space
            float scale_x = (float)image.width / (float)GetScreenWidth();
            float scale_y = (float)image.height / (float)GetScreenHeight();

            int x1 = (int)(initial_mouse_position.x * scale_x);
            int y1 = (int)(initial_mouse_position.y * scale_y);
            int x2 = (int)(current_mouse_position.x * scale_x);
            int y2 = (int)(current_mouse_position.y * scale_y);

            int left = x1 < x2 ? x1 : x2;
            int right = x1 > x2 ? x1 : x2;
            int top = y1 < y2 ? y1 : y2;
            int bottom = y1 > y2 ? y1 : y2;

            int width = abs(right - left);
            int height = abs(top - bottom);
            uint8_t *cropped_image_pixels = (uint8_t *)malloc(width * height * bytes_per_pixel);

            unsigned char *pixels = image.data;
            for (int y = top; y < bottom; y++) {
                unsigned char *src = pixels + (y * image.width + left) * bytes_per_pixel;
                unsigned char *dst = cropped_image_pixels + ((y - top) * width * bytes_per_pixel);
                memcpy(dst, src, width * bytes_per_pixel);
            }

            Image cropped = image;
            cropped.data = cropped_image_pixels;
            cropped.height = height;
            cropped.width = width;
            int image_size = 0;

            // ExportImage(cropped, "image.png");
            uint8_t *data = ExportImageToMemory(cropped, ".png", &image_size);
            UnloadFileData(data);
            free(cropped_image_pixels);
#if defined(__linux__)
            FILE *pipe = popen("wl-copy --type image/png", "w");
            fwrite(data, 1, image_size, pipe);
#elif defined(__APPLE__)
            copy_png_to_clipboard(data, image_size);
#endif

            break;
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

        // Draw selected region
        Vector2 top_left = initial_mouse_position;
        Vector2 bottom_right = current_mouse_position;

        Vector2 top_right = {
            bottom_right.x,
            top_left.y,
        };

        Vector2 bottom_left = {
            top_left.x,
            bottom_right.y,
        };

        int dash_size = 15;
        int gap = 15;
        Color color = RED;

        if (action == ACTION_CAPTURE) {
            DrawLineDashed(top_left, top_right, dash_size, gap, color);
            DrawLineDashed(top_right, bottom_right, dash_size, gap, color);
            DrawLineDashed(bottom_right, bottom_left, dash_size, gap, color);
            DrawLineDashed(bottom_left, top_left, dash_size, gap, color);
        }

        EndDrawing();
    }

    UnloadTexture(texture);
    CloseWindow();
    free(screenshot.pixels);

    return 0;
}

// pipeline
// take screenshot - different regions
// save destination - whats the filename?
