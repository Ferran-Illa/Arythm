#ifndef COMMON_H
#define COMMON_H

#include <stdio.h> 
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <float.h> // Added for DBL_MAX

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

typedef struct {
    int rows;
    int cols;
    double *data;
} Matrix;

typedef struct {
    int size;
    double *data;
} Vector;

typedef struct {
    double step_size;
    int num_steps;
    double num_points;
    bool plot_bifurcation_diagram;
    double initial_t;
    double initial_y[3];
    double param[14];
    double excitation[3];
    double bifurcation[3];
} InputParams;

typedef struct{
    double step_size;
    int num_steps;
    double initial_t;
    double initial_y[3];
    double param[14];
    double excitation[3];
} OdeFunctionParams;

#define MAT(m, i, j) (m.data[(i) * (m.cols) + (j)]) // Access element at (i, j), zero-indexed!!
#define VEC(v, i) (v.data[i]) // Access element at i, zero-indexed!!

typedef void (*ODEFunction)(double t, double *y, double *dydt, double *param, double *excitation_control); // Ensure ODEFunction matches ODE_func signature
// Represents a function for solving ordinary differential equations (ODEs),
// where 't' is the independent variable (time) and 'y' is the dependent variable.


#endif // COMMON_H