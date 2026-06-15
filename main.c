#include "capture.h"
#include "raylib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    struct Screenshot screenshot = { 0 };

#if defined(__linux__)
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
#elif defined(__APPLE__)
    screenshot = capture_screen();
#elif defined(_WIN32)
#else
#error Unsupported platform
#endif

    int pixel_format = 0;
#if defined(__linux__)
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8;
#elif defined(__APPLE__)
    pixel_format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
#endif

    Image image = {
        .data = screenshot.pixels,
        .width = screenshot.width,
        .height = screenshot.height,
        .mipmaps = 1,
        .format = pixel_format,
    };

    SetTraceLogLevel(LOG_ERROR);
    SetConfigFlags(FLAG_WINDOW_HIGHDPI);

    InitWindow(1, 1, "Screenshot");
    ToggleBorderlessWindowed();

    Texture2D texture = LoadTextureFromImage(image);
    printf("screen: %d x %d\n", GetScreenWidth(), GetScreenHeight());
    printf("render: %d x %d\n", GetRenderWidth(), GetRenderHeight());

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        Rectangle src = { 0, 0, (float)texture.width, (float)texture.height };
        Rectangle dst = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
        DrawTexturePro(texture, src, dst, (Vector2){ 0, 0 }, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadTexture(texture);
    CloseWindow();
    free(screenshot.pixels);

    return 0;
}
