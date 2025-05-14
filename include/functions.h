// This code is part of the SingleCell project.
#include "common.h"

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

    #ifndef ALGEBRA_H
        extern Matrix matrix_product(const Matrix *a, const Matrix *b);
        extern Vector create_vector(int size);
        extern Matrix create_matrix(int rows, int cols);
        extern void free_vector(Vector *vec);
        extern void free_matrix(Matrix *mat);
    #endif // ALGEBRA_H

    #ifndef ODE_H
        extern void ODE_func(double t, double *y, double *dydt, double *function_param, double *ode_param, bool no_excitation);
        extern Matrix euler_integration_multidimensional(ODEFunction ode_func, OdeFunctionParams ode_settings);
        extern int diffusion1D(OdeFunctionParams* ode_input, DiffusionData* diffusion_data, int frames);    
    #endif // ODE_H 

#endif // FUNCTIONS_H