#ifndef ODE_H
#define ODE_H

#include "include/common.h"
#include "include/functions.h"

// ---------------------------- ODE SOLVER ---------------------------

Matrix euler_integration(ODEFunction ode_func, double step_size, int num_steps, double initial_t, double initial_y) {
    double t = initial_t;
    double y = initial_y;
    Matrix result = create_matrix(2, num_steps); // 2 rows, for t and y

    for (int i = 0; i < num_steps; i++) {
        MAT(result, 0, i) = t;
        MAT(result, 1, i) = y;
        //printf("Step %d: t = %f, y = %f\n", i, t, y); // If we need to print all steps
        y += step_size * ode_func(t, y);
        t += step_size;
    }
    return result;
}

double expODE(double t, double y) {
    return -y; // dy/dt = -y
}


#endif // ODE_H