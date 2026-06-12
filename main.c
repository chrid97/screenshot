#include "raylib.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct PpmImage {
    int width;
    int height;
    // unsigned char vs u8?
    unsigned char *pixels;
};

int main(void) {
    // int result = system("grim screenshot.png");
    // if (result != 0) {
    //     fprintf(stderr, "failed to take screenshot\n");
    //     return 1;
    // }

    FILE *file = popen("grim -t ppm -", "r");
    struct PpmImage pp_image = { 0 };

    char magic[3];
    int max_ppm_value = 0;
    fscanf(file, "%2s", magic);
    // check if ppm image p6
    fscanf(file, "%d %d", &pp_image.width, &pp_image.height);
    fscanf(file, "%d", &max_ppm_value);
    fgetc(file); // consume newline after 255

    // why use size_t not int?
    size_t size = pp_image.width * pp_image.height * 3;
    pp_image.pixels = malloc(size);
    fread(pp_image.pixels, 1, size, file);

    pclose(file);

    Image image = { .data = pp_image.pixels,
                    .width = pp_image.width,
                    .height = pp_image.height,
                    .mipmaps = 1,
                    .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 };

    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_UNDECORATED | FLAG_VSYNC_HINT);
    InitWindow(1, 1, "Screenshot");
    Texture2D texture = LoadTextureFromImage(image);
    ToggleBorderlessWindowed();

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(BLACK);

        float scale_x = (float)GetScreenWidth() / texture.width;
        float scale_y = (float)GetScreenHeight() / texture.height;
        float scale = scale_x < scale_y ? scale_x : scale_y;

        DrawTextureEx(texture, (Vector2){ 0, 0 }, 0.0f, scale, WHITE);

        EndDrawing();
    }

    UnloadTexture(texture);
    CloseWindow();

    free(pp_image.pixels);

    return 0;
}
