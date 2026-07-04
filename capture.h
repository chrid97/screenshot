#pragma once

#include <stddef.h>
#include <stdint.h>

enum {
    BYTES_PER_PIXEL = 4,
};

typedef struct {
    int width;
    int height;
    // int scale; // (CG) do i need this
    uint8_t *pixels;
} Screenshot;

Screenshot capture_screen(void);

// (CG) should this return success status?
void copy_png_to_clipboard(const uint8_t *data, size_t size);

static inline size_t screenshot_pixel_count(const Screenshot *s) {
    return (size_t)(s->height * s->width);
};

static inline size_t screenshot_byte_count(const Screenshot *s) {
    return BYTES_PER_PIXEL * (size_t)(s->height * s->width);
}
