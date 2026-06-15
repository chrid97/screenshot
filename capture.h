#pragma once

struct Screenshot {
    int width;
    int height;
    int stride;
    int scale;
    unsigned char *pixels;
};

struct Screenshot capture_screen(void);
