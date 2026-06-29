#pragma once

typedef struct {
    int width;
    int height;
    int stride;
    int scale;
    unsigned char *pixels;
} Screenshot;

Screenshot capture_screen(void);

void copy_png_to_clipboard(const unsigned char *data, int size);
