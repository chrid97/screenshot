#include "capture.h"
#include "raylib.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

Color CHARCOAL_OLIVE = { 26, 26, 20, 255 };
Color MIDNIGHT_BLUE = { 23, 56, 98, 255 };
Color ALABASTER = { 244, 241, 234, 255 };
Color HOTKEY = { 150, 145, 135, 255 }; // #969187

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}
static inline int min_int(int a, int b) { return a < b ? a : b; }

static inline int max_int(int a, int b) { return a > b ? a : b; }

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

// (CG) Compile time constants for default values?
Color stroke_color = RED;
int stroke_width = 2;

int main(void) {
    CaptureMode capture_mode = CAPTURE_MODE_REGION;
    Draw action = ACTION_CAPTURE;
    size_t size = 0;
    int pixel_format = 0;
    int bytes_per_pixel = 0;

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

    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
    bytes_per_pixel = 3;
#elif defined(__APPLE__)
    screenshot = capture_screen();
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    bytes_per_pixel = 4;
#elif defined(_WIN32)
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    bytes_per_pixel = 4;
    screenshot = capture_screen();
#else
#error Unsupported platform
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
    Vector2 previous_mouse_position = { 0 };

    while (!WindowShouldClose()) {
        float scale_x = (float)image.width / (float)GetScreenWidth();
        float scale_y = (float)image.height / (float)GetScreenHeight();

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
            printf("Initial mouse position %f, %f\n", GetMousePosition().x, GetMousePosition().y);
        }

        if (mouse_down) {
            previous_mouse_position = current_mouse_position;
            current_mouse_position = GetMousePosition();
        }

        int curr_x = current_mouse_position.x * scale_x;
        int curr_y = current_mouse_position.y * scale_y;
        int initial_x = initial_mouse_position.x * scale_x;
        int initial_y = initial_mouse_position.y * scale_y;

        if (mouse_down && action == ACTION_FREEHAND) {
            int prev_x = previous_mouse_position.x * scale_x;
            int prev_y = previous_mouse_position.y * scale_y;

            ImageDrawLine(&image, prev_x, prev_y, curr_x, curr_y, stroke_color);
            UpdateTexture(texture, image.data);
        }

        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            switch (action) {
            case ACTION_FREEHAND:
                break;
            case ACTION_RECTANGLE: {
                ImageDrawRectangleLines(&image,
                                        initial_x,
                                        initial_y,
                                        curr_x - initial_x,
                                        curr_y - initial_y,
                                        stroke_color);
                UpdateTexture(texture, image.data);
                break;
            }
            case ACTION_LINE: {
                ImageDrawLine(&image, initial_x, initial_y, curr_x, curr_y, stroke_color);
                UpdateTexture(texture, image.data);
                break;
            };
            case ACTION_CAPTURE: {
                // ON macos screenshots are in retina space
                // mouse is in screen space
                // we want to scale mosue coords up to retina space
                int left = min_int(initial_x, curr_x);
                int right = max_int(initial_x, curr_x);
                int top = min_int(initial_y, curr_y);
                int bottom = max_int(initial_y, curr_y);

                int width = abs(right - left);
                int height = abs(bottom - top);
                uint8_t *cropped_image_pixels = (uint8_t *)malloc(width * height * bytes_per_pixel);

                unsigned char *pixels = image.data;
                for (int y = top; y < bottom; y++) {
                    unsigned char *src = pixels + (y * image.width + left) * bytes_per_pixel;
                    unsigned char *dst =
                        cropped_image_pixels + ((y - top) * width * bytes_per_pixel);
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
                pclose(pipe);
#elif defined(__APPLE__)
                copy_png_to_clipboard(data, image_size);
#endif
                UnloadFileData(data);
                free(cropped_image_pixels);
                // (CG) maybe add a done variable that we set to true here
                // while (windowlose && !done)
                CloseWindow();
                return 0;
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

        // Draw selected region
        // (CG) this assumes i only ever start a rect from the topleft
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
        Color color = BLACK;

        if (mouse_down && action == ACTION_CAPTURE) {
            DrawLineDashed(top_left, top_right, dash_size, gap, color);
            DrawLineDashed(top_right, bottom_right, dash_size, gap, color);
            DrawLineDashed(bottom_right, bottom_left, dash_size, gap, color);
            DrawLineDashed(bottom_left, top_left, dash_size, gap, color);
        }

        if (mouse_down && action == ACTION_LINE) {
            DrawLine(initial_mouse_position.x,
                     initial_mouse_position.y,
                     current_mouse_position.x,
                     current_mouse_position.y,
                     stroke_color);
        }

        if (mouse_down && action == ACTION_RECTANGLE) {
            Rectangle rect = {
                initial_mouse_position.x,
                initial_mouse_position.y,
                (current_mouse_position.x - initial_mouse_position.x),
                (current_mouse_position.y - initial_mouse_position.y),
            };
            DrawRectangleRoundedLinesEx(rect, 0.1f, 8, stroke_width, stroke_color);
        }

        // ============================================================================
        // UI Overlay
        // ============================================================================
        const int button_count = 4;

        const float button_size = 36.0f;
        const float icon_size = 16.0f;
        const float padding = 4.0f;
        const float gap1 = 5.0f;
        const float top_margin = 10.0f;

        const float toolbar_w =
            padding * 2 + button_count * button_size + (button_count - 1) * gap1;
        const float toolbar_h = padding * 2 + button_size;

        Rectangle toolbar = {
            .x = GetScreenWidth() / 2.0f - toolbar_w / 2.0f,
            .y = top_margin,
            .width = toolbar_w,
            .height = toolbar_h,
        };

        DrawRectangleRounded(toolbar, 0.35f, 8, CHARCOAL_OLIVE);

        for (int i = 0; i < button_count; i++) {
            Rectangle button = {
                .x = toolbar.x + padding + i * (button_size + gap1),
                .y = toolbar.y + padding,
                .width = button_size,
                .height = button_size,
            };

            Draw action_for_button = ACTION_CAPTURE;
            if (i == 1)
                action_for_button = ACTION_RECTANGLE;
            if (i == 2)
                action_for_button = ACTION_LINE;
            if (i == 3)
                action_for_button = ACTION_FREEHAND;

            if (action == action_for_button) {
                DrawRectangleRounded(button, 0.35f, 8, MIDNIGHT_BLUE);
            }

            Vector2 center = {
                button.x + button.width / 2.0f,
                button.y + button.height / 2.0f,
            };

            if (action_for_button == ACTION_RECTANGLE) {
                Rectangle icon = {
                    center.x - icon_size / 2.0f,
                    center.y - icon_size / 2.0f,
                    icon_size,
                    icon_size,
                };
                DrawRectangleRoundedLinesEx(icon, 0.15f, 4, 1.0f, ALABASTER);
            }

            if (action_for_button == ACTION_LINE) {
                DrawLineEx((Vector2){ center.x - 8, center.y + 8 },
                           (Vector2){ center.x + 8, center.y - 8 },
                           1.5f,
                           ALABASTER);
            }

            if (action_for_button == ACTION_FREEHAND) {
                Vector2 points[] = {
                    { center.x - 8, center.y + 1 }, { center.x - 5, center.y - 4 },
                    { center.x - 2, center.y - 2 }, { center.x + 1, center.y + 4 },
                    { center.x + 4, center.y - 1 }, { center.x + 8, center.y + 2 },
                };

                for (int i = 0; i < 5; i++) {
                    DrawLineEx(points[i], points[i + 1], 1.5f, ALABASTER);
                }
            }

            if (action_for_button == ACTION_CAPTURE) {
                Vector2 top_left = { center.x - 8, center.y - 6 };
                Vector2 top_right = { center.x + 8, center.y - 6 };
                Vector2 bottom_right = { center.x + 8, center.y + 6 };
                Vector2 bottom_left = { center.x - 8, center.y + 6 };

                DrawLineDashed(top_left, top_right, 3, 2, ALABASTER);
                DrawLineDashed(top_right, bottom_right, 3, 2, ALABASTER);
                DrawLineDashed(bottom_right, bottom_left, 3, 2, ALABASTER);
                DrawLineDashed(bottom_left, top_left, 3, 2, ALABASTER);
            }

            char label[2];
            label[0] = '1' + i;
            label[1] = '\0';

            int font_size = 10;
            int text_width = MeasureText(label, font_size);

            DrawText(label,
                     button.x + button.width - text_width - 3,
                     button.y + button.height - font_size - 2,
                     font_size,
                     HOTKEY);
        }
        EndDrawing();
    }

    UnloadTexture(texture);
    free(screenshot.pixels);
    CloseWindow();

    return 0;
}
