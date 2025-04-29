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

Vector read_matrix_row(Matrix *matrix, int row){ // Reads a row of a Matrix as if it was a vector, DOES NOT MAKE A COPY!
    Vector output;
    output.size = matrix->cols;
    output.data = matrix->data + row * matrix->cols; // Point to the start of the row
    return output;
}

void help_display() {
    printf("Usage: ./SingleCell.sh [OPTIONS]\n");
    printf("Options:\n");
    printf("  -s <step_size>       Specify the step size for the ODE solver (default: 0.05).\n");
    printf("  -n <num_steps>       Specify the number of steps for the ODE solver (default: 2000).\n");
    printf("  -t <initial_t>       Specify the initial time value (default: 0.0).\n");
    printf("  -y <y1> <y2> <y3>    Specify the initial values for the ODE system (default: 0.2, 0.0, 0.0).\n");
    printf("  -param <p1> ... <p16> Specify the 16 parameters for the ODE system (default: predefined values).\n");
    printf("  -h, -help            Display this help message and exit.\n");
    printf("\nExamples (default):\n");
    printf("  ./SingleCell.sh -s 0.05 -n 20000 -t 0.0 -y 0.2 0.0 0.0 -param 3.33 9 8 250 60 0.395 9 33.33 29 15 0.5 0.13 0.04 .1 2 100\n");
    printf("\nDescription:\n");
    printf("This program solves a system of ordinary differential equations (ODEs) using the Euler method.\n");
    printf("You can customize the solver's behavior using the options above.\n");
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
    int num_steps = 20000;

    double initial_t = 0.0;
    double initial_y[] = {0.2, 0.0, 0.0};
        // param=[tv+, tv1-, tv2-, tw+, tw-, td, t0, tr, tsi, k, Vsic, Vc, Vv, J_exc, T_exc, T_tot]
    double param[16] = {3.33, 9, 8, 250, 60, .395, 9, 33.33, 29, 15, .5, .13, .04, .2, 1.0, 300}; // Example parameters set 6


    for (int i  = 1; i < argc; i++){
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            step_size = atof(argv[++i]);
        } else if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_steps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            initial_t = atof(argv[++i]);
        } else if (strcmp(argv[i], "-y") == 0 && i + 3 < argc) {
            for (int j = 0; j < 3; j++) {
                initial_y[j] = atof(argv[++i]);
            }
        } else if (strcmp(argv[i], "-param") == 0 && i + 16 < argc) {
            for (int j = 0; j < 16; j++) {
                param[j] = atof(argv[++i]);
            }
        } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
            help_display();
            return 0;
        }
    }

    Matrix result= euler_integration_multidimensional(ODE_func, step_size, num_steps, initial_t, initial_y, 3, param);
    //print_matrix(&result); // Print the matrix for debugging
    /* 
        Cast the first row (time) to t and the second row (ode values) to y, 
        vectors are read linearly which makes casting rows to vectors feasible.
        Temporary solution until a proper vectorization is implemented.  
    */
    Vector t = read_matrix_row(&result, 0); // Time data is stored in the first row
    Vector y = read_matrix_row(&result, 1); // ODE Voltage values are stored in the second row

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
