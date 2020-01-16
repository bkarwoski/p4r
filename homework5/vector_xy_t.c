#include <stdlib.h>
#include "vector_xy_t.h"

vector_xy_t vector_create(void) {
    vector_xy_t v;
    v.size = 0;
    v.capacity = 8;
    v.xData = malloc(sizeof(double) * v.capacity);
    v.yData = malloc(sizeof(double) * v.capacity);
    return v;
}

void vector_append(vector_xy_t *vec, double x, double y) {
    if (vec->size >= vec->capacity) {
        vec->capacity *= 2;
        double *newxData = realloc(vec->xData, sizeof(double) * vec->capacity);
        double *newyData = realloc(vec->yData, sizeof(double) * vec->capacity);

        vec->xData = newxData;
        vec->yData = newyData;
    }
    vec->xData[vec->size] = x;
    vec->yData[vec->size] = y;
    vec->size++;
}

void vector_delete(vector_xy_t *vec) {
    free(vec->xData);
    free(vec->yData);
}
