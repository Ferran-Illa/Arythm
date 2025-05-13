#include "include/common.h"
#include "include/functions.h"
#include "include/plotting.h"
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


void ODE_func(double t, double *y, double *dydt, double* param, double *excitation) { // Represents a function for solving ordinary differential equations (ODEs)
    
    // volatile states this should be stored in RAM, as these values are temporary
    // All heaviside functions are replaced by if statements.

    volatile double Volt; // Voltage, there is a 1uF/cm2 capacitor in the membrane, ommited due to the 1.
    volatile double vdt;
    volatile double wdt;

    //excitation control variables
    const double J_exc = param[13];// excitation current

    const double T_exc = excitation[0]; //excitation duration
    const double T_tot = excitation[1]; // total period between excitations

    static double t_start = 0; // Store the initial time
    double t_diff;

    if(t_start < 0) // If the time since the last excitation is negative, reset it to 0
    { t_start = 0; }
        
    if(y[0] >= param[11]) // Action of p = 1
    {
        // Seems like 1/param[7] should be multiplied by y[0], possibly a mistake in the original code?
        Volt = y[1] * (y[0]-param[11]) * (1-y[0]) / param[5] - 1/param[7] + mIsi(y, param); // V = (- Ifi - Iso - Isi )/ Cm, sign cancellations have been made
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

    if(t_diff <= T_exc) // T_exc makes the excitation activate at the start of the period.
    { Volt += J_exc; } // If the excitation is active, add the current to the voltage

    if(t_diff >= T_tot)
    { t_start = t; } // Reset the timer

    dydt[0] = Volt; // dV/dt
    dydt[1] = vdt; // dv/dt
    dydt[2] = wdt; // dw/dt
}


// ---------------------------- ODE SOLVER ---------------------------


Matrix euler_integration_multidimensional(ODEFunction ode_func, OdeFunctionParams params) {
    
    int     num_steps   = params.num_steps;
    double  step_size   = params.step_size;
    double  *param      = params.param;
    double  *excitation = params.excitation;

    double  *y  = params.initial_y; // Initial conditions
    double  t   = params.initial_t;
    
    int dim = 3; // Number of dimensions (variables) in the ODE system
    double dydt[dim]; // Derivatives

    Matrix result = create_matrix(dim + 1, num_steps); // rows: 1 for t, dim for y

    for (int i = 0; i < num_steps; i++) {
        MAT(result, 0, i) = t; // Store time
        for (int j = 0; j < dim; j++) {
            MAT(result, j + 1, i) = y[j]; // Store y values
        }

        // Compute derivatives
        ode_func(t, y, dydt, param, excitation); // Call the ODE function to compute derivatives

        // Update y using Euler's method
        for (int j = 0; j < dim; j++) {
            y[j] += step_size * dydt[j];
        }

        t += step_size; // Update time
    }
    return result;
}

void diffusion1D(OdeFunctionParams* ode_input, DiffusionData* diffusion_data) {
    // Extract parameters from the input structure
    int rows = diffusion_data->rows;
    int cols = diffusion_data->cols;
    double time = diffusion_data->time;
    Vector *M_voltage = diffusion_data->M_voltage;
    Vector *M_vgate = diffusion_data->M_vgate;
    Vector *M_wgate = diffusion_data->M_wgate;
    double diffusion = diffusion_data->diffusion;
    double cell_size = diffusion_data->cell_size;
    int excited_cells = diffusion_data->excited_cells;

    // Loop over the grid points
    double Prev_Voltage = VEC(*M_voltage, rows-1); // Periodic boundary condition, before i = 0 comes i = rows-1 (the last cell)
    for (int i = 0; i < rows-1; i++) {
        
        double y[3] = {VEC(*M_voltage, i), VEC(*M_vgate, i), VEC(*M_wgate, i)}; // Casting to fit required type for Ode_func
        double dydt[3]; // Derivatives
        
        if(i >= excited_cells){time = -1;} // If the cell is not excited, set time to -1 to avoid excitation

        ODE_func(time, y, dydt, ode_input->param, ode_input->excitation); // Call the ODE function to compute derivatives
        
        dydt[0] += ( VEC(*M_voltage, i+1) - 2*VEC(*M_voltage, i) + Prev_Voltage )* diffusion / pow(cell_size, 2); // 1D Diffusion term for voltage
        
        Prev_Voltage = VEC(*M_voltage, i); // Store the unupdated voltage value
        M_voltage   -> data[i] += dydt[0] * ode_input->step_size; // Update voltage
        M_vgate     -> data[i] += dydt[1] * ode_input->step_size; // Update vgate
        M_wgate     -> data[i] += dydt[2] * ode_input->step_size; // Update wgate

    }
    // Fulfill the non-flux (neumann's boundary condition) at the edges of the grid
    M_voltage   -> data[rows-1] = M_voltage -> data[0]; // Periodic boundary condition
    M_vgate     -> data[rows-1] = M_vgate   -> data[0]; // Update vgate
    M_wgate     -> data[rows-1] = M_wgate   -> data[0]; // Update wgate
}
// End of ODE_H guard
#endif