#pragma once
#include <stdlib.h>

typedef struct vector_xy {
    size_t size;
    size_t capacity;
    double *xData;
    double *yData;
} vector_xy_t;

vector_xy_t vector_create(void);

void vector_append(vector_xy_t *vec, double x, double y);

void vector_delete(vector_xy_t *vec);
