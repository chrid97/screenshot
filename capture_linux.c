#import "capture.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

Screenshot capture_screen() {
    Screenshot screenshot = { 0 };
    FILE *file = popen("grim -t ppm -", "r");
    char magic[3];

    int max_ppm_value = 0;
    fscanf(file, "%2s", magic);
    fscanf(file, "%d %d", &screenshot.width, &screenshot.height);
    fscanf(file, "%d", &max_ppm_value);
    fgetc(file);

    // Convert from RGB to RGBA
    size_t total_pixels = (size_t)(screenshot.width * screenshot.height);
    screenshot.pixels = (uint8_t *)malloc(total_pixels * BYTES_PER_PIXEL);
    for (size_t i = 0; i < total_pixels; i++) {
        fread(&screenshot.pixels[i * BYTES_PER_PIXEL], 1, 3, file);
        screenshot.pixels[i * BYTES_PER_PIXEL + 3] = 255;
    }

    pclose(file);

    return screenshot;
}
