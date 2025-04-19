#ifndef COMMON_H
#define COMMON_H

#include <stdio.h> 
#include <stdlib.h>
#include <SDL2/SDL.h>

typedef struct {
    int rows;
    int cols;
    double *data;
} Matrix;

typedef struct {
    int size;
    double *data;
} Vector;

#define MAT(m, i, j) (m.data[(i) * (m.cols) + (j)]) // Access element at (i, j), zero-indexed!!
#define VEC(v, i) (v.data[i]) // Access element at i, zero-indexed!!


typedef double (*ODEFunction)(double t, double y);
// Represents a function for solving ordinary differential equations (ODEs),
// where 't' is the independent variable (time) and 'y' is the dependent variable.

#endif // COMMON_H