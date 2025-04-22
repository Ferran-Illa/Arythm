// This code is part of the SingleCell project.
#include "common.h"

#ifndef ALGEBRA_H
    extern Matrix matrix_product(const Matrix *a, const Matrix *b);
    extern Vector create_vector(int size);
    extern Matrix create_matrix(int rows, int cols);
    extern void free_vector(Vector *vec);
    extern void free_matrix(Matrix *mat);
#endif // ALGEBRA_H

#ifndef UI_HEADER_H
    extern void plot_with_sdl(const Vector *x, const Vector *y, const double *axes, const double x_tick, const double y_tick);
    extern void plot(const Vector *x, const Vector *y);
    extern void print_matrix(const Matrix *mat);
#endif // UI_HEADER_H

#ifndef ODE_H
    extern void ODE_func(double t, double *y, double *dydt, double *param);
    extern Matrix euler_integration_multidimensional(ODEFunction ode_func, double step_size, int num_steps, double initial_t, double *initial_y, int dim, double *param);    
#endif // ODE_H 