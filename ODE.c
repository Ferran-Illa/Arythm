#ifndef ODE_H
#define ODE_H

#include "include/common.h"
#include "include/functions.h"
#include "include/ODE.h" 
 // --------------------------- ODE DEFINITION -------------------------
 // param=[tv+, tv1-, tv2-, tw-, td, t0, tr, tsi, k, Vsic, Vc, Vv]
 // y=[V, v, w]
 // p= H(V-param[10])   ; q= H(V-param[11])

double H(double x) {
    if (x < 0) {
        return 0.0; // Return 0 for negative inputs
    } else {
        return 1.0; // Return 1 for non-negative inputs
    }
}
double tauvminus(double *y, double *param) {
        return (1 - H(y[0] - param[11])) * param[1] + H(y[0] - param[11]) * param[2];
    }

double Ifi(double *y, double *param) {
        return ( -y[1]*H(y[0]-param[10])*(y[0]-param[10])*(1-y[0])/param[4]);
    }

double Iso(double *y, double *param) {
        return ( y[0]*(1-H(y[0]-param[10]))/param[5]+H(y[0]-param[10])/param[6]);
    }

double Isi(double *y, double *param) {
        return ( -y[2]*(1+tanh(param[8]*(y[0]-param[9])))/(2*param[7]));
    }


void ODE_func(double t, double *y, double *dydt, double *param) { // Represents a function for solving ordinary differential equations (ODEs)
    dydt[0] = - Ifi(y, param) - Iso(y, param) - Isi(y, param);
    dydt[1] = (1 - H(y[0] - param[10])) * (1 - y[1]) / tauvminus(y, param) - H(y[0] - param[10]) * y[1] / param[0];
}





 
// ---------------------------- ODE SOLVER ---------------------------

typedef void (*ODEFunction)(double, double *, double *, double *); // Ensure ODEFunction matches ODE_func signature


Matrix euler_integration_multidimensional(ODEFunction ODE_func, double step_size, int num_steps, double initial_t, double *initial_y, int dim, double *param) {
    double t = initial_t;
    double y[dim]; // Current state
    double dydt[dim]; // Derivatives
    for (int i = 0; i < dim; i++) {
        y[i] = initial_y[i]; // Initialize y
    }

    Matrix result = create_matrix(dim + 1, num_steps); // rows: 1 for t, dim for y

    for (int i = 0; i < num_steps; i++) {
        MAT(result, 0, i) = t; // Store time
        for (int j = 0; j < dim; j++) {
            MAT(result, j + 1, i) = y[j]; // Store y values
        }

        // Compute derivatives
        ode_func(t, y, dydt, param); // Call the ODE function to compute derivatives

        // Update y using Euler's method
        for (int j = 0; j < dim; j++) {
            y[j] += step_size * dydt[j];
        }

        t += step_size; // Update time
    }
    return result;
}

// End of ODE_H guard
#endif