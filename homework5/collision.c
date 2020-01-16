#include "collision.h"
#include <stdio.h>
bool intersects(double x1, double y1, double x2, double y2,
                double x3, double y3, double x4, double y4) {
    double vec1[2] = {x2 - x1, y2 - y1};
    double t11[2] = {x3 - x1, y3 - y1};
    double t12[2] = {x4 - x1, y4 - y1};
    double cv1t11 = vec1[0] * t11[1] - vec1[1] * t11[0];
    double cv1t12 = vec1[0] * t12[1] - vec1[1] * t12[0];
    bool neg1 = (cv1t11 * cv1t12 <= 0);

    double vec2[2] = {x4 - x3, y4 - y3};
    double t21[2] = {x1 - x3, y1 - y3};
    double t22[2] = {x2 - x3, y2 - y3};
    double cv2t21 = vec2[0] * t21[1] - vec2[1] * t21[0];
    double cv2t22 = vec2[0] * t22[1] - vec2[1] * t22[0];
    bool neg2 = (cv2t21 * cv2t22 <= 0);

    bool intersected = (neg1 && neg2) && !((cv1t11 * cv1t12 == 0) && (cv2t21 * cv2t22 == 0));
    return intersected;
}

bool contains(double x, double y, vector_xy_t *polyX) {
    int posCount = 0;
    int negCount = 0;
    for (int i = 0; i < polyX->size; i++) {
        double x1 = polyX->xData[i];
        double y1 = polyX->yData[i];
        double x2 = polyX->xData[(i + 1) % polyX->size];
        double y2 = polyX->yData[(i + 1) % polyX->size];
        double vec1[2] = {x2 - x1, y2 - y1};
        double t1[2] = {x - x1, y - y1};
        double cross = vec1[0] * t1[1] - vec1[1] * t1[0];
        if (cross < 0) {
            negCount++;
        } else if (cross > 0) {
            posCount++;
        }
    }
    if (negCount > 0 && posCount > 0) {
        return false;
    }
    return true;
}

bool collision(vector_xy_t *poly1, vector_xy_t *poly2) {
    for (int k = 0; k < poly1->size; k++) {
        double x1 = poly1->xData[k];
        double y1 = poly1->yData[k];
        double x2 = poly1->xData[(k + 1) % poly1->size];
        double y2 = poly1->yData[(k + 1) % poly1->size];
        for (int j = 0; j < poly2->size; j++) {
            double x3 = poly2->xData[j];
            double y3 = poly2->yData[j];
            double x4 = poly2->xData[(j + 1) % poly2->size];
            double y4 = poly2->yData[(j + 1) % poly2->size];
            if (intersects(x1, y1, x2, y2, x3, y3, x4, y4)) {
                return true;
            }
        }
    }
    return contains(poly1->xData[0], poly1->yData[0], poly2) ||
           contains(poly2->xData[0], poly2->yData[0], poly1);
}
