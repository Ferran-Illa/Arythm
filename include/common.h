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

    bool plot_bifurcation_diagram;
    bool plot_singlecell_potential;
    bool plot_1D;
    bool plot_2D;

    int num_steps;
    int frame_speed;
    int tissue_size[2];
    int excited_cells[2];
    int excited_cells_pos[2];

    double step_size;
    double num_points;
    double initial_t;
    double initial_y[3];
    double param[14];
    double excitation[3];
    double bifurcation[3];
    double diffusion;
    double cell_size;
} InputParams;

typedef struct{
    double step_size;
    int num_steps;
    double initial_t;
    double initial_y[3];
    double param[14];
    double excitation[3];
} OdeFunctionParams;

typedef struct {
    double time;
    Matrix *M_voltage;
    Matrix *M_vgate;
    Matrix *M_wgate;
    double diffusion;
    double cell_size;
    int excited_cells[2];
    int excited_cells_pos[2];
} DiffusionData;

#define MAT(m, i, j) ((m).data[(i) * ((m).cols) + (j)]) // Access element at (i, j), zero-indexed!!
#define VEC(v, i) ((v).data[i]) // Access element at i, zero-indexed!!

typedef void (*ODEFunction)(double t, double *y, double *dydt, double *param, double *excitation_control, bool no_excitation); // Ensure ODEFunction matches ODE_func signature
// Represents a function for solving ordinary differential equations (ODEs),
// where 't' is the independent variable (time) and 'y' is the dependent variable.

typedef int (*DiffVideo)(OdeFunctionParams* ode_input, DiffusionData* diffusion_data, int frames); // Function pointer type for diffusion functions

#endif // COMMON_H