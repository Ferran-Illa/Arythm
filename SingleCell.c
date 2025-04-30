#include "include/common.h"
#include "include/functions.h"
#include "include/plotting.h"

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

void plot_bifurcation(double *t_exc_values, double *t_tot_values, int num_points) {
    Plot plot;
    Vector t_exc, t_tot;

    // Initialize vectors
    t_exc.size = num_points;
    t_tot.size = num_points;
    t_exc.data = t_exc_values;
    t_tot.data = t_tot_values;

    // Create the plot
    single_plot(&plot, &t_exc, &t_tot, "Bifurcation Diagram", "T_exc", "T_tot");
}

Vector find_values(Vector time_t , Vector y_t, int num_steps, double step_size, double t_tot_min) {
    Vector ans= create_vector((int)(2*num_steps*step_size/t_tot_min));
    bool STATE=1;
    for (int i = 0; i < num_steps; i++) {
        if (VEC(y_t, i) > 0.13 && STATE) {
            VEC(ans, i) = VEC(time_t, i);
            STATE=!STATE;
        }
        if (VEC(y_t, i) < 0.13 && STATE ) {
            VEC(ans, i) = VEC(time_t, i);
            STATE=!STATE;
        }
    }
    return ans;
}

void help_display() {
    printf("Usage: ./SingleCell.sh [OPTIONS]\n");
    printf("Options:\n");
    printf("  -stp <step_size>       Specify the step size for the ODE solver (default: 0.05).\n");
    printf("  -nstp <num_steps>       Specify the number of steps for the ODE solver (default: 2000).\n");
    printf("  -npt <num_points>       Specify the number of points for the bifurcation diagram (default: 100).\n");
    printf("  -t <initial_t>       Specify the initial time value (default: 0.0).\n");
    printf("  -y <V> <v> <w>    Specify the initial values for the ODE system (default: 0.2, 0.0, 0.0).\n");
    printf("  -param <p1> ... <p14> Specify the 14 parameters for the ODE system (default: predefined values).\n");
    printf("  -exc <exc_time> <T_tot_min> <T_tot_max>  Specify the excitation  (default: predefined).\n");
    printf("  -h, -help            Display this help message and exit.\n");
    printf("\nExamples (default):\n");
    printf("  ./SingleCell.sh -s 0.05 -n 20000 -t 0.0 -y 0.2 0.0 0.0 -param 3.33 9 8 250 60 0.395 9 33.33 29 15 0.5 0.13 0.04 .1  -exc 2 70 300\n");
    printf("\nDescription:\n");
    printf("This program solves a system of ordinary differential equations (ODEs) using the Euler method.\n");
    printf("You can customise the solver's behaviour using the options above.\n");
}

int single_plot(Plot *plot, Vector *x, Vector *y, char *title, char *x_label, char *y_label) {
    
    plot_init(plot); // Initialize the plot
    // Initialize plot properties
    strcpy(plot->title, title);
    strcpy(plot->x_label, x_label);
    strcpy(plot->y_label, y_label);

    // Add series to the plot
    plot_add_series(plot, x, y, title,
        (Color){0, 0, 0, 255}, // Black
        LINE_SOLID, MARKER_NONE, 1, 1, PLOT_LINE);

    // Show the plot
    PlotError error = plot_show(plot);
    if (error != PLOT_SUCCESS) {
        fprintf(stderr, "Error showing plot: %d\n", error);
        return -1;
    }

    // Clean up
    plot_cleanup(plot);

    return 0;
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
    int num_steps = 30000;
    double num_points = 100; // Number of points for the bifurcation diagram

    double initial_t = 0.0;
    double initial_y[] = {0.0, .9, .9};
        // param=[tv+, tv1-, tv2-, tw+, tw-, td, t0, tr, tsi, k, Vsic, Vc, Vv, J_exc]
    //double param[14] = {3.33, 9, 8, 250, 60, .395, 9, 33.33, 29, 15, .5, .13, .04, 1}; // Example parameters set 6
    double param[14] = {3.33, 15.6, 5, 350, 80, .407, 9, 34, 26.5, 15, .45, .15, .04, 1}; // Example parameters set 4
    double excitation[3]={1, 50, 300}; // Default Periodic excitation parameters [T_exc, T_tot_min, T_tot_max]

    
    // Input parsing
    for (int i  = 1; i < argc; i++){
        if (strcmp(argv[i], "-stp") == 0 && i + 1 < argc) {
            step_size = atof(argv[++i]);
        } else if (strcmp(argv[i], "-nstp") == 0 && i + 1 < argc) {
            num_steps = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            initial_t = atof(argv[++i]);
        } else if (strcmp(argv[i], "-y") == 0 && i + 3 < argc) {
            for (int j = 0; j < 3; j++) {
                initial_y[j] = atof(argv[++i]);
            }
        } else if (strcmp(argv[i], "-param") == 0 && i + 14 < argc) {
            for (int j = 0; j < 14; j++) {
                param[j] = atof(argv[++i]);
            }
        } else if(strcmp(argv[i], "-exc") == 0 && i + 3 < argc) {
            for (int j = 0; j < 3; j++) {
                excitation[j] = atof(argv[++i]);
            }
        } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
            help_display();
            return 0;
        } else if (strcmp(argv[i], "-npt") == 0 && i + 1 < argc) {
            num_points = atoi(argv[++i]);
        }

        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            help_display();
            return 1;
        }
    }


    double t_tot_min = excitation[1];
    double t_tot_max = excitation[2];
    double t_tot_step = (t_tot_max - t_tot_min) / (num_points - 1); // Step size for total excitation duration

    Vector t_exc_values;
    Vector t_tot_values; 

    // Loop over T_exc values
    for (int i = 0; i < num_points; i++) {
        excitation[1] = t_tot_min + i * (t_tot_max - t_tot_min) / (num_points - 1); // T_exc
        excitation[0] = 1.5; // Set a fixed T_exc for now

        // Solve the ODE system
        Matrix result_t = euler_integration_multidimensional(ODE_func, step_size, num_steps, initial_t, initial_y, 3, param, excitation);

        Vector t_t = read_matrix_row(&result_t, 0); // Time data is stored in the first row
        Vector y_t = read_matrix_row(&result_t, 1); // ODE Voltage values are stored in the second row

        Vector find_values;
        
        // Analyze the result to compute T_tot (this is an example, adjust as needed)

        free_matrix(&result_t); // Free the matrix after use, vectors are freed too with this action.
        
    }

    Matrix result= euler_integration_multidimensional(ODE_func, step_size, num_steps, initial_t, initial_y, 3, param, excitation);
    //print_matrix(&result); // Print the matrix for debugging
    /* 
        Cast the first row (time) to t and the second row (ode values) to y, 
        vectors are read linearly which makes casting rows to vectors feasible.
        Temporary solution until a proper vectorization is implemented.  
    */

    /*
    Vector t = read_matrix_row(&result, 0); // Time data is stored in the first row
    Vector y = read_matrix_row(&result, 1); // ODE Voltage values are stored in the second row

    // Plot the results
    Plot Alternance;
    single_plot(&Alternance, &t, &y, "Alternance", "Time (s)", "Voltage (V)");

    free_matrix(&result); // Free the matrix after use, vectors are freed too with this action.
    return 0;
    */

    // Plot the bifurcation diagram
    Plot bifurcationPlot;
    single_plot(&bifurcationPlot, &t_exc_values, &t_tot_values, "Bifurcation Diagram", "T_exc", "T_tot");
    free_matrix(&result); // Free the result matrix

    return 0;

    }

    