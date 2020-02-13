#pragma once
#include <stdbool.h>
#include "vector_xy_t.h"

bool intersects(double x1, double y1, double x2, double y2,
                double x3, double y3, double x4, double y4);

bool contains(double x, double y, vector_xy_t *polyX);

bool collision(vector_xy_t *poly1, vector_xy_t *poly2);