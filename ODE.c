#include "include/common.h"
#include "include/functions.h"
#include <math.h>

#ifndef ODE_H
#define ODE_H

 // --------------------------- ODE DEFINITION -------------------------
 // param=[tv+, tv1-, tv2-, tw+, tw-, td, t0, tr, tsi, k, Vsic, Vc, Vv, J_exc]
 // y=[V, v, w]
 // p= H(V-param[11])   ; q= H(V-param[12])
 // ode_param=[ T_exc, T_tot]


double mIsi(double *y, double *param) 
{
    return ( y[2]*(1 + tanh( param[9]*(y[0]-param[10]) ) ) / (2*param[8]) );
}


void ODE_func(double t, double *y, double *dydt, const double *param, const double *ode_param) { // Represents a function for solving ordinary differential equations (ODEs)
    
    // volatile states this should be stored in RAM, as these values are temporary
    // All heaviside functions are replaced by if statements.

    volatile double Volt; // Voltage, there is a 1uF/cm2 capacitor in the membrane, ommited due to the 1.
    volatile double vdt;
    volatile double wdt;

    //excitation control variables
    const double T_exc = ode_param[0]; //excitation duration
    const double J_exc = param[13];// excitation current
    const double T_tot = ode_param[1]; // total period between excitations

    static double t_start = 0; // excitation starting time
    double t_diff;

        
    if(y[0] >= param[11]) // Action of p = 1
    {
        // Seems like 1/param[7] should be multiplied by y[0], possibly a mistake in the original code?
        Volt = y[1] * (y[0]-param[11]) * (1-y[0]) / param[5] - 1/param[7] + mIsi(y, param) + param[13]; // V = (- Ifi - Iso - Isi )/ Cm, sign cancellations have been made
        vdt =  - y[1] / param[0];
        wdt =  - y[2] / param[3];
    }
    else // p = 0
    {
        Volt = - y[0]/param[6] + mIsi(y, param); // V = (- Ifi - Iso - Isi )/ Cm, but Ifi = 0
        wdt =  (1 - y[2]) / param[4];

        if(y[0] >= param[12]) // Action of tauvminus (q = 1)
        {
            vdt = (1 - y[1]) / param[2];
        }
        else // q = 0
        {
            vdt = (1 - y[1]) / param[1];
        }
    }

    t_diff = t - t_start; // Calculate the time difference since the last excitation
    if(t_diff <= T_exc)
    { Volt += J_exc; } // If the excitation is active, add the current to the voltage
    else if(t_diff >= T_tot)
    { t_start = t; } // Reset the timer


    dydt[0] = Volt; // dV/dt
    dydt[1] = vdt; // dv/dt
    dydt[2] = wdt; // dw/dt
}


// ---------------------------- ODE SOLVER ---------------------------


Matrix euler_integration_multidimensional(ODEFunction ode_func, double step_size, int num_steps, double initial_t, double *initial_y, int dim, double *param) {
    
    double t = initial_t;
    double y[dim]; // Current state
    double dydt[dim]; // Derivatives

    for (int i = 0; i < dim; i++) {
        y[i] = initial_y[i]; // Initialize y // Maybe initial_y is redundant if it is directly substituted by y.
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