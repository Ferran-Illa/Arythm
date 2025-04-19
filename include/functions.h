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
    extern void plot_with_sdl(const Vector *x, const Vector *y);
    extern void plot(const Vector *x, const Vector *y);
    extern void print_matrix(const Matrix *mat);
#endif // UI_HEADER_H

#ifndef ODE_H
    extern Matrix euler_integration(ODEFunction ode_func, double step_size, int num_steps, double initial_t, double initial_y);
    extern double expODE(double t, double y);
#endif // ODE_H