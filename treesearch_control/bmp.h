#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DWORD uint32_t
#define WORD uint16_t
#define LONG int32_t

// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-tagbitmapfileheader
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
} __attribute__((__packed__)) BITMAPFILEHEADER;

// https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-tagbitmapinfoheader
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} __attribute__((__packed__)) BITMAPINFOHEADER;

typedef struct bgr {
    uint8_t b;
    uint8_t g;
    uint8_t r;
} __attribute__((__packed__)) color_bgr_t;

typedef struct bitmap {
    int width;
    int height;
    color_bgr_t *data;
} bitmap_t;

// calculate the number of bytes of memory needed to serialize the bitmap
// that is, to write a valid bmp file to memory
size_t bmp_calculate_size(bitmap_t *bmp);

// write the bmp file to memory at data, which must be at least
// bmp_calculate_size large.
void bmp_serialize(bitmap_t *bmp, uint8_t *data);
