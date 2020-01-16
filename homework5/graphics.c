#include <unistd.h>
#include <stdbool.h>
#include <math.h>
#include "bmp.h"
#include "image_server.h"
#include "vector_xy_t.h"
#include "graphics.h"

vector_xy_t gx_rasterize_line(int x0, int y0, int x1, int y1) {
    vector_xy_t perimeter = vector_create();
    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    int e2 = 0;
    while (true) {
        vector_append(&perimeter, x0, y0);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
    return perimeter;
}

void gx_draw(bitmap_t *bmp, color_bgr_t color, vector_xy_t *points) {
    int index = 0;
    for (int i = 0; i < points->size; i++) {
        if (points->xData[i] >= 0 && points->yData[i] >= 0) {
            index = (int)points->xData[i] + (int)points->yData[i] * 640;
            bmp->data[index] = color;
        }
    }
}

void gx_draw_line(bitmap_t *bmp, color_bgr_t color, int x0, int y0, int x1, int y1) {
    vector_xy_t line = gx_rasterize_line(x0, y0, x1, y1);
    gx_draw(bmp, color, &line);
    vector_delete(&line);
}

void gx_draw_poly(bitmap_t *bmp, color_bgr_t color, vector_xy_t *shape) {
    roundC(shape);
    for (int i = 0; i < shape->size; i++) {
        int x0 = (int)shape->xData[i];
        int y0 = (int)shape->yData[i];
        int x1 = (int)shape->xData[(i + 1) % shape->size];
        int y1 = (int)shape->yData[(i + 1) % shape->size];
        gx_draw_line(bmp, color, x0, y0, x1, y1);
    }
}

void roundC(vector_xy_t *doubles) {
    double xMin = doubles->xData[0];
    double yMin = doubles->yData[0];
    double epsilon = 1e-6;
    int32_t newX = 0;
    int32_t newY = 0;
    for (int i = 1; i < doubles->size; i++) {
        if (doubles->xData[i] < xMin) {
            xMin = doubles->xData[i];
        }
        if (doubles->yData[i] < yMin) {
            yMin = doubles->yData[i];
        }
    }
    for (int i = 0; i < doubles->size; i++) {
        if (doubles->xData[i] == xMin) {
            newX = ceil(doubles->xData[i]);
        } else {
            newX = floor(doubles->xData[i] - epsilon);
        }
        if (doubles->yData[i] == yMin) {
            newY = ceil(doubles->yData[i]);
        } else {
            newY = floor(doubles->yData[i] - epsilon);
        }
        doubles->xData[i] = newX;
        doubles->yData[i] = newY;
    }
}

void gx_trans(double x, double y, vector_xy_t *vec) {
    for (int i = 0; i < vec->size; i++) {
        vec->xData[i] = vec->xData[i] + x;
        vec->yData[i] = vec->yData[i] + y;
    }
}

void gx_rot(double theta, vector_xy_t *vec) {
    double c = cos(theta);
    double s = sin(theta);
    for (int i = 0; i < vec->size; i++) {
        double newX = c * vec->xData[i] + s * vec->yData[i];
        double newY = -s * vec->xData[i] + c * vec->yData[i];
        vec->xData[i] = newX;
        vec->yData[i] = newY;
    }
}

vector_xy_t gx_rect(double width, double height) {
    vector_xy_t rect = vector_create();
    vector_append(&rect, width / 2, height / 2);
    vector_append(&rect, -width / 2, height / 2);
    vector_append(&rect, -width / 2, -height / 2);
    vector_append(&rect, width / 2, -height / 2);
    return rect;
}

vector_xy_t gx_rob(void) {
    double height = 80 / 4.0;
    double width = height * 4 / 3.0;
    vector_xy_t tri = vector_create();
    vector_append(&tri, width / 2, 0);
    vector_append(&tri, -width / 2, height / 2);
    vector_append(&tri, -width / 2, -height / 2);
    return tri;
}

void gx_fill_poly(bitmap_t *bmp, color_bgr_t color, vector_xy_t *shape) {
    int height = 480;
    int xmin[height];
    int xmax[height];
    for (int i = 0; i < height; i++) {
        xmin[i] = -1;
        xmax[i] = -1;
    }
    roundC(shape);
    for (int i = 0; i < shape->size; i++) {
        int x0 = (int)shape->xData[i];
        int y0 = (int)shape->yData[i];
        int x1 = (int)shape->xData[(i + 1) % shape->size];
        int y1 = (int)shape->yData[(i + 1) % shape->size];
        vector_xy_t line = gx_rasterize_line(x0, y0, x1, y1);
        for (int j = 0; j < line.size; j++) {
            if (line.xData[j] >= 0 && line.yData[j] >= 0) {
                int x = (int)line.xData[j];
                int y = (int)line.yData[j];
                if (xmin[y] == -1) {
                    xmin[y] = x;
                    xmax[y] = x;
                } else {
                    xmin[y] = (int)fmin(xmin[y], x);
                    xmax[y] = (int)fmax(xmax[y], x);
                }
            }
        }
        vector_delete(&line);
    }
    for (int y = 0; y < height; y++) {
        if (xmin[y] != -1) {
            for (int x = xmin[y]; x <= xmax[y]; x++) {
                bmp->data[x + y * bmp->width] = color;
            }
        }
    }
}
