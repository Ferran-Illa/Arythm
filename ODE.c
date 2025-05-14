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


void ODE_func(double t, double *y, double *dydt, double* param, double *excitation, bool no_excitation) { // Represents a function for solving ordinary differential equations (ODEs)
    
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

    if(t_diff <= T_exc && !no_excitation) // T_exc makes the excitation activate at the start of the period.
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
        ode_func(t, y, dydt, param, excitation, 0); // Call the ODE function to compute derivatives

        // Update y using Euler's method
        for (int j = 0; j < dim; j++) {
            y[j] += step_size * dydt[j];
        }

        t += step_size; // Update time
    }
    return result;
}

int diffusion1D(OdeFunctionParams* ode_input, DiffusionData* diffusion_data, int frames) {

    if(frames <= 0) {
        printf("ERROR: The number of frames must be positive.\n");
        return -1;
    }

    // Extract parameters from the input structure
    int cols            = diffusion_data -> M_voltage -> cols; // Number of columns in the matrix
    double time_copy    = diffusion_data -> time;
    Matrix *M_voltage   = diffusion_data -> M_voltage;
    Matrix *M_vgate     = diffusion_data -> M_vgate;
    Matrix *M_wgate     = diffusion_data -> M_wgate;
    double diffusion    = diffusion_data -> diffusion;
    double cell_size    = diffusion_data -> cell_size;
    int excited_cells   = diffusion_data -> excited_cells[0];

    bool no_excitation = true; // Flag to control excitation, avoid excitations by default
    int i = 0; // For the 1D diffusion, we only need to loop over the columns.

    for(int f = 0; f < frames; f++){
        // VEC reads the first and only row of the matrices. Should work. Might be weird.
        
        time_copy = diffusion_data->time; // Update the time for the ODE function
        double Prev_Voltage = MAT(*M_voltage, i, 0); // Periodic boundary condition, before i = 1 comes (i = 0) (the first cell)
        for (int j = 1; j < cols-1; j++) {
        
            double y[3] = {MAT(*M_voltage, i,j), MAT(*M_vgate, i,j), MAT(*M_wgate, i,j)}; // Casting to fit required type for Ode_func
            double dydt[3]; // Derivatives
            
            if(j < excited_cells){ // && (time_copy <= ode_input->excitation[0]) ){
                no_excitation = false; // Cells to be excited once
            }
            else{
                no_excitation = true; // Cells not to be excited
            }

            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function to compute derivatives
            
            dydt[0] += ( MAT(*M_voltage, i, j+1) - 2*MAT(*M_voltage, i, j) + Prev_Voltage )* diffusion / pow(cell_size, 2); // 1D Diffusion term for voltage
            
            Prev_Voltage = MAT(*M_voltage, i, j); // Store the unupdated voltage value

            MAT(*M_voltage, i, j)   += dydt[0] * ode_input->step_size; // Update voltage
            MAT(*M_vgate, i, j)     += dydt[1] * ode_input->step_size; // Update vgate
            MAT(*M_wgate, i, j)     += dydt[2] * ode_input->step_size; // Update wgate

        }
        // Fulfill the non-flux boundary conditions at the edges of the grid
        M_voltage   -> data[cols-1] = M_voltage -> data[cols-2]; // Periodic boundary condition
        M_vgate     -> data[cols-1] = M_vgate   -> data[cols-2]; // Update vgate
        M_wgate     -> data[cols-1] = M_wgate   -> data[cols-2]; // Update wgate

        M_voltage   -> data[0]     = M_voltage -> data[1]; // Periodic boundary condition
        M_vgate     -> data[0]     = M_vgate   -> data[1]; // Update vgate
        M_wgate     -> data[0]     = M_wgate   -> data[1]; // Update wgate
        
        diffusion_data->time += ode_input->step_size;
    }
    // Loop over the grid points
    return 0;
}

int diffusion2D(OdeFunctionParams* ode_input, DiffusionData* diffusion_data, int frames) {
    // Implement the 2D diffusion logic here
    // This is a placeholder for the actual implementation
    if(frames <= 0) {
        printf("ERROR: The number of frames must be positive.\n");
        return -1;
    }

    // Extract parameters from the input structure
    int rows            = diffusion_data -> M_voltage -> rows; // Number of rows in the matrix
    int cols            = diffusion_data -> M_voltage -> cols; // Number of columns in the matrix
    double time_copy    = diffusion_data -> time;
    Matrix *M_voltage   = diffusion_data -> M_voltage;
    Matrix *M_vgate     = diffusion_data -> M_vgate;
    Matrix *M_wgate     = diffusion_data -> M_wgate;
    double diffusion    = diffusion_data -> diffusion;
    double cell_size    = diffusion_data -> cell_size;
    int excited_cells_x = diffusion_data -> excited_cells[0];
    int excited_cells_y = diffusion_data -> excited_cells[1];

    bool no_excitation = true; // Flag to control excitation, avoid excitations by default
    
    for (int f = 0; f < frames; f++) {
        time_copy = diffusion_data->time; // Update the time for the ODE function
        
        // Create a copy of the voltage matrix to avoid overwriting during updates
        Matrix voltage_copy = copy_matrix(M_voltage); // Can be optimized as we only require two memorized columns

        for (int i = 1; i < rows-1; i++) {
            for (int j = 1; j < cols-1; j++) {
                double y[3] = {MAT(voltage_copy, i, j), MAT(*M_vgate, i, j), MAT(*M_wgate, i, j)};
                double dydt[3]; // Derivatives

                if (i < excited_cells_y && j < excited_cells_x) { // Excitation condition
                    no_excitation = false; // Cells to be excited once
                } else {
                    no_excitation = true; // Cells not to be excited
                }

                ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

                // Compute the 9-point Laplacian for voltage
                double laplacian = 0.0;

                // Direct neighbors
                double top      = MAT(voltage_copy, i - 1, j);
                double left     = MAT(voltage_copy, i, j - 1);

                double bottom   = MAT(voltage_copy, i + 1, j);
                double right    = MAT(voltage_copy, i, j + 1);

                // Diagonal neighbors
                double top_left     = MAT(voltage_copy, i - 1, j - 1);
                double top_right    = MAT(voltage_copy, i - 1, j + 1);
                double bottom_left  = MAT(voltage_copy, i + 1, j - 1);
                double bottom_right = MAT(voltage_copy, i + 1, j + 1);

                // Weighted sum for the 9-point Laplacian
                laplacian = (4 * MAT(voltage_copy, i, j) +
                             2 * (top + bottom + left + right) +
                             (top_left + top_right + bottom_left + bottom_right)) / (6 * pow(cell_size, 2));


                // Add the diffusion term to the voltage derivative
                dydt[0] += diffusion * laplacian;
                /*
                if(j == 1 && i == 1){
                    //printf("Debugging");
                    if(MAT(voltage_copy, i, j) > 0.5){
                        printf("Debugging");
                    }
                    if(MAT(voltage_copy, i, j) < 0.5){
                        printf("Debugging");
                    }
                }
                */

                // Update the matrices
                MAT(*M_voltage, i, j)   += dydt[0] * ode_input->step_size;
                MAT(*M_vgate, i, j)     += dydt[1] * ode_input->step_size;
                MAT(*M_wgate, i, j)     += dydt[2] * ode_input->step_size;
            }
        }

        // Update the edges of the grid
        for (int j = 1; j < cols-1; j++) {
            // Fulfill the non-flux boundary conditions at the edges of the grid
            double dydt[3]; // Derivatives
            
            // Update the gates
            double y[3] = {MAT(voltage_copy, 0, j), MAT(*M_vgate, 0, j), MAT(*M_wgate, 0, j)};
            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, 0, j)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, 0, j)     += dydt[2] * ode_input->step_size;

            y[0] = MAT(voltage_copy, rows-1, j);
            y[1] = MAT(*M_vgate, rows-1, j);
            y[2] = MAT(*M_wgate, rows-1, j);

            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, rows-1, j)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, rows-1, j)     += dydt[2] * ode_input->step_size;
            
            // Update the voltage
            MAT(*M_voltage, 0, j)    = MAT(*M_voltage, 1, j); // Top edge
            MAT(*M_voltage, rows-1, j) = MAT(*M_voltage, rows-2, j); // Bottom edge
        }

        for (int i = 1; i < rows-1; i++) {
            // Fulfill the non-flux boundary conditions at the edges of the grid
            double dydt[3]; // Derivatives
            
            // Update the gates
            double y[3] = {MAT(voltage_copy, i, 0), MAT(*M_vgate, i, 0), MAT(*M_wgate, i, 0)};
            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, i, 0)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, i, 0)     += dydt[2] * ode_input->step_size;

            y[0] = MAT(voltage_copy, i, cols-1);
            y[1] = MAT(*M_vgate, i, cols-1);
            y[2] = MAT(*M_wgate, i, cols-1);

            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, i, cols-1)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, i, cols-1)     += dydt[2] * ode_input->step_size;
            
            // Update the voltage
            MAT(*M_voltage, i, 0)     = MAT(*M_voltage, i, 1); // Left edge
            MAT(*M_voltage, i, cols-1) = MAT(*M_voltage, i, cols-2); // Right edge
        }

        double dydt[3]; // Derivatives
        double y[3];
        // Handle the corners for non-flux boundary conditions
        MAT(*M_voltage, 0, 0) = MAT(*M_voltage, 1, 1); // Top-left corner

            y[0] = MAT(voltage_copy, 0, 0);
            y[1] = MAT(*M_vgate, 0, 0);
            y[2] = MAT(*M_wgate, 0, 0);
            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, 0, 0)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, 0, 0)     += dydt[2] * ode_input->step_size;

        MAT(*M_voltage, 0, cols - 1) = MAT(*M_voltage, 1, cols - 2); // Top-right corner

            y[0] = MAT(voltage_copy, 0, cols - 1);
            y[1] = MAT(*M_vgate, 0, cols - 1);
            y[2] = MAT(*M_wgate, 0, cols - 1);
            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, 0, cols - 1)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, 0, cols - 1)     += dydt[2] * ode_input->step_size;

        MAT(*M_voltage, rows - 1, 0) = MAT(*M_voltage, rows - 2, 1); // Bottom-left corner

            y[0] = MAT(voltage_copy, rows - 1, 0);
            y[1] = MAT(*M_vgate, rows - 1, 0);
            y[2] = MAT(*M_wgate, rows - 1, 0);
            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, rows - 1, 0)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, rows - 1, 0)     += dydt[2] * ode_input->step_size;

        MAT(*M_voltage, rows - 1, cols - 1) = MAT(*M_voltage, rows - 2, cols - 2); // Bottom-right corner
            
            y[0] = MAT(voltage_copy, rows - 1, cols - 1);
            y[1] = MAT(*M_vgate, rows - 1, cols - 1);
            y[2] = MAT(*M_wgate, rows - 1, cols - 1);
            ODE_func(time_copy, y, dydt, ode_input->param, ode_input->excitation, no_excitation); // Call the ODE function

            MAT(*M_vgate, rows - 1, cols - 1)     += dydt[1] * ode_input->step_size;
            MAT(*M_wgate, rows - 1, cols - 1)     += dydt[2] * ode_input->step_size;

        // Update the time
        diffusion_data->time += ode_input->step_size;

        // Free the copied matrix
        free_matrix(&voltage_copy);
    }

    return 0;
}
// End of ODE_H guard
#endif