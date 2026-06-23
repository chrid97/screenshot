#include "capture.h"
#include "raylib.h"
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
    struct Screenshot screenshot = { 0 };
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
    size_t size = screenshot.width * screenshot.height * 3;
    screenshot.pixels = malloc(size);
    fread(screenshot.pixels, 1, size, file);
    pclose(file);

    double t_capture_end = now_ms();
    printf("capture: %.2f ms\n", t_capture_end - t_capture_start);
#elif defined(__APPLE__)
    screenshot = capture_screen();
#elif defined(_WIN32)
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
#endif
    double t_window_start = now_ms();
    Image image = {
        .data = screenshot.pixels,
        .width = screenshot.width,
        .height = screenshot.height,
        .mipmaps = 1,
        .format = pixel_format,
    };

    SetTraceLogLevel(LOG_ERROR);
    // SetConfigFlags(FLAG_WINDOW_HIGHDPI);

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

    // return 0;
    // switch (capture_mode) {
    // case CAPTURE_MODE_SCREEN: {
    //     ExportImage(image, "image.png");
    //     return 0;
    // } break;
    // case CAPTURE_MODE_REGION: {
    // } break;
    // case CAPTURE_MODE_WINDOW: {
    // } break;
    // }
    //
    Vector2 initial_mouse_position = { 0 };
    Vector2 current_mouse_position = { 0 };
    while (!WindowShouldClose()) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            initial_mouse_position = GetMousePosition();
            printf("%f, %f\n", GetMousePosition().x, GetMousePosition().y);
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            current_mouse_position = GetMousePosition();
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
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
            unsigned char *cropped_image_pixels = malloc(width * height * bytes_per_pixel);

            unsigned char *pixels = image.data;
            for (int y = top; y < bottom; y++) {
                printf("%d\n", y * image.width + x1);
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
#if defined(__linux__)
            FILE *pipe = popen("wl-copy --type image/png", "w");
            fwrite(data, 1, image_size, pipe);
#elif defined(__APPLE__)
            copy_png_to_clipboard(data, image_size);
#endif
            break;
        }

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
        DrawLineDashed(top_left, top_right, dash_size, gap, color);
        DrawLineDashed(top_right, bottom_right, dash_size, gap, color);
        DrawLineDashed(bottom_right, bottom_left, dash_size, gap, color);
        DrawLineDashed(bottom_left, top_left, dash_size, gap, color);

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
//
