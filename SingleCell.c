#include "include/common.h"
#include "include/functions.h"

// ----------------------------- MAIN -----------------------------

void main()
{
    // Example usage of the euler method ODE solver
    double step_size = 0.1;
    int num_steps = 10;
    double initial_t = 0.0;
    double initial_y = 10.0;
    Matrix M = euler_integration(expODE, step_size, num_steps, initial_t, initial_y);

    print_matrix(&M); // Print the matrix for debugging

    /* 
        Cast the first row (time) to t and the second row (ode values) to y, 
        vectors are read linearly which makes casting rows to vectors feasible.
        Temporary solution untila proper vectorization is implemented.  
    */
    Vector t; Vector y;
    t.size = num_steps;
    y.size = num_steps;
    t.data = M.data;
    y.data = M.data + num_steps; // Offset to the second row
    // Plot the results
    plot_with_sdl(&t, &y);
    free_matrix(&M); // Free the matrix after use, vectors are freed too with this action.
}