#include "include/common.h"
#include "include/functions.h"

// ----------------------------- MAIN -----------------------------

void main()
{
    // Example usage of the euler method ODE solver
    double step_size = .05; 
    int num_steps = 500;

    double initial_t = 0.0;
    double initial_y[] = {0.0, .0, .0};
        // param=[     tv+, tv1-, tv2-, tw+, tw-, td, t0, tr,   tsi, k, Vsic, Vc, Vv]
    double param[13] = {3.33, 12,   2, .1, .1, .362, 50, 33.33, .029, 1.5, .7, .13, .04}; // Example parameters

    Matrix result= euler_integration_multidimensional(ODE_func, step_size, num_steps, initial_t, initial_y, 3, param);
    //print_matrix(&result); // Print the matrix for debugging
    /* 
        Cast the first row (time) to t and the second row (ode values) to y, 
        vectors are read linearly which makes casting rows to vectors feasible.
        Temporary solution until a proper vectorization is implemented.  
    */
    Vector t; Vector y;
    t.size = num_steps;
    y.size = num_steps;
    t.data = result.data;
    y.data = result.data + num_steps; // Offset to the second row
    // Plot the results

    double x_tick = num_steps*step_size/10; // Example: 5ms spacing
    double y_tick = .1; // Example: 10mV spacing
    double axes[4] = {0, step_size*num_steps, 0.0, 1.0}; // x_min, x_max, y_min, y_max

    print_vector(&t);
    print_vector(&y);

    plot_with_sdl(&t, &y, axes, x_tick, y_tick);
    free_matrix(&result); // Free the matrix after use, vectors are freed too with this action.
 
    // Axes are not well set
    }
