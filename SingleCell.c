#include "include/common.h"
#include "include/functions.h"
#include "include/ODE.h" // Include the header file for ODE_func

// ----------------------------- MAIN -----------------------------

void main()
{
    // Example usage of the euler method ODE solver
    double step_size = 0.1;
    int num_steps = 10;
    double initial_t = 0.0;
    double initial_y[] = {10.0, 10.0, 10.0};
    double param[] = {3.33, 10.0, 3.33, 3.33, 3.33, 3.33, 10.0, 13.03, 3.33, 10}; // Example parameters
    Matrix M = euler_integration(ODE_H, step_size, num_steps, initial_t, initial_y[0]);

    print_matrix(&M); // Print the matrix for debugging

    /* 
        Cast the first row (time) to t and the second row (ode values) to y, 
        vectors are read linearly which makes casting rows to vectors feasible.
        Temporary solution untila proper vectorization is implemented.  
    
    Vector t; Vector y;
    t.size = num_steps;
    y.size = num_steps;
    t.data = M.data;
    y.data = M.data + num_steps; // Offset to the second row
    // Plot the results
    // plot_with_sdl(&t, &y);
    // free_matrix(&M); // Free the matrix after use, vectors are freed too with this action.
    */
    Matrix result= euler_integration_multidimensional(ODE_H, step_size, num_steps, initial_t, initial_y, 3, param);
    }
