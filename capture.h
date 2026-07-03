#pragma once

typedef struct {
    int width;
    int height;
    int stride; // (CG) I don't use this so i can probably remove it
    int scale;
    unsigned char *pixels;
} Screenshot;

const int bytes_per_pixel = 4;
Screenshot capture_screen(void);

void copy_png_to_clipboard(const unsigned char *data, int size);
