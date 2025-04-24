#include "include/common.h"
#include "include/functions.h"

// ----------------------------- MAIN -----------------------------

double array_max(double *arr, int size) {
    double max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

double array_min(double *arr, int size) {
    double min = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] < min) {
            min = arr[i];
        }
    }
    return min;
}

int main(int argc, char *argv[])
{
    // Example usage of the euler method ODE solver

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    double step_size = 0.05; 
    int num_steps = 2000;

    double initial_t = 0.0;
    double initial_y[] = {0.2, 0.0, 0.0};
        // param=[     tv+, tv1-, tv2-, tw+, tw-, td, t0, tr,   tsi, k, Vsic, Vc, Vv]
    double param[13] = {3.33, 9, 8, 250, 60, .395, 9, 33.33, 29, 15, .5, .13, .04}; // Example parameters set 6


    for (int i  = 1; i < argc; i++){
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            step_size = atof(argv[++i]);
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_steps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            initial_t = atof(argv[++i]);
        } else if (strcmp(argv[i], "-y") == 0 && i + 1 < argc) {
            for (int j = 0; j < 3; j++) {
                initial_y[j] = atof(argv[++i]);
            }
        } else if (strcmp(argv[i], "-param") == 0 && i + 1 < argc) {
            for (int j = 0; j < 13; j++) {
                param[j] = atof(argv[++i]);
            }
        }
    }

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

    const double y_min = array_min(y.data, num_steps);
    const double y_max = array_max(y.data, num_steps);

    double axes[4] = {0, step_size*num_steps, y_min, y_max}; // x_min, x_max, y_min, y_max
    double x_tick = num_steps*step_size/10; // Example: 10 divisions
    double y_tick = (y_max - y_min)/10; // Example: 10 divisions

    // Plot the results
    //print_vector(&t);
    //print_vector(&y);

    plot_with_sdl(&t, &y, axes, x_tick, y_tick);
    free_matrix(&result); // Free the matrix after use, vectors are freed too with this action.
    return 0;
    // Axes are not well set
    }
