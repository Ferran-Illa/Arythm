#include "include/common.h"
#include "include/functions.h"
#include "include/plotting.h"

// ----------------------------- MAIN -----------------------------

double array_max(double *arr, int size) { // Maybe move to algebra.c??
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

/* 
    Cast the matrix row to a vector.
    Vectors are read linearly which makes casting rows to vectors feasible.
*/
Vector read_matrix_row(Matrix *matrix, int row){ // Reads a row of a Matrix as if it was a vector, DOES NOT MAKE A COPY!
    Vector output;
    output.size = matrix->cols;
    output.data = matrix->data + row * matrix->cols; // Point to the start of the row
    return output;
}

// It first finds an upwards crossing point (y>threshold) and then a downwards crossing point (y<threshold) to find the APD and DP values.
Vector find_values(const Vector time_t , const Vector y_t, int num_excitations, int num_steps, double threshold) {
    Vector ans= create_vector(2*num_excitations);
    bool STATE=1;
    int j = 0;
    for (int i = 0; i < num_steps; i++) {
        if (VEC(y_t, i) > threshold && STATE) {
            VEC(ans, j) = VEC(time_t, i);
            STATE=!STATE;
            j++;
        }
        if (VEC(y_t, i) < threshold && !STATE ) {
            VEC(ans, j) = VEC(time_t, i);
            STATE=!STATE;+
            j++;
        }
        if (j >= 2*num_excitations) {
            break; // Stop if we have found enough crossing points
        }
    }
    ans.size = j; // Update the size of the vector to the number of crossing points found
    return ans;
}

void help_display() {
    printf("Usage: ./SingleCell.sh [OPTIONS]\n");
    printf("Options:\n");
    printf("  -stp <step_size>       Specify the step size for the ODE solver (default: 0.05).\n");
    printf("  -nstp <num_steps>      Specify the number of steps for the ODE solver (default: 30000).\n");
    printf("  -npt <num_points>      Specify the number of points for the bifurcation diagram (default: 100).\n");
    printf("  -t <initial_t>         Specify the initial time value (default: 0.0).\n");
    printf("  -y <V> <v> <w>         Specify the initial values for the ODE system (default: 0.0, 0.9, 0.9).\n");
    printf("  -param <p1> ... <p14>  Specify the 14 parameters for the ODE system (default: predefined values).\n");
    printf("  -exc <exc_time> <T_tot> Specify the excitation parameters (default: 1, 300).\n");
    printf("  -h, -help              Display this help message and exit.\n");
    printf("  -bif                   Plot the bifurcation diagram.\n");
    printf("  -bif_set <T_exc> <T_tot_min> <T_tot_max> Specify bifurcation parameters (default: 1, 300, 400).\n");
    printf("\nExamples (default):\n");
    printf("  ./SingleCell.sh -stp 0.05 -nstp 30000 -t 0.0 -y 0.0 0.9 0.9 -param 3.33 15.6 5 350 80 0.407 9 34 26.5 15 0.45 0.15 0.04 1 -exc 1 300 400 -bif\n");
    printf("\nDescription:\n");
    printf("This program solves a system of ordinary differential equations (ODEs) using the Euler method.\n");
    printf("You can customise the solver's behaviour using the options above.\n");
}

int single_plot(Plot *plot, Vector *x, Vector *y, char *title, char *x_label, char *y_label, PlotType plot_type) {
    
    plot_init(plot); // Initialize the plot
    // Initialize plot properties
    strcpy(plot->title, title);
    strcpy(plot->x_label, x_label);
    strcpy(plot->y_label, y_label);

    // Add series to the plot
    plot_add_series(plot, x, y, title,
        (Color){0, 0, 0, 255}, // Black
        LINE_SOLID, MARKER_NONE, 1, 2, plot_type);

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


void bifurcation_diagram(double *excitation, int num_points, double step_size, int num_steps, double initial_t, double *initial_y, double *param) {
    double t_tot_min = excitation[1];
    double t_tot_max = excitation[2];
    double t_tot_step = (t_tot_max - t_tot_min) / (num_points - 1); // Step size for total excitation duration

    int skip_excitations = 20; // Number of excitations to skip at start of T_exc.
    int num_excitations = 5; // Number of excitations to consider for each T_exc.

    Vector APD = create_vector(num_points); // Create a vector to store the APD values.
    Vector DP = create_vector(num_points); // Create a vector to store the DP values.

    int total_excitations = 0; // Total number of excitations found so far
    // Loop over T_exc values
    for (int i = 0; i < num_points; i++) {
        excitation[1] = t_tot_min + i * t_tot_step; // T_exc

        num_steps = (int)(num_excitations*excitation[1] / step_size -1); // Update num_steps based on the new T_exc, allowing for 10 pulses.

        // Solve the ODE system
        Matrix result_t = euler_integration_multidimensional(ODE_func, step_size, num_steps, initial_t, initial_y, 3, param, excitation);

        Vector t_t = read_matrix_row(&result_t, 0); // Time data is stored in the first row
        Vector y_t = read_matrix_row(&result_t, 1); // ODE Voltage values are stored in the second row

        Vector cross_points = find_values(t_t, y_t, num_excitations, num_steps, param[11]); // Find the crossing points for the first 10 pulses
           
        /* 
         * Ignore the first 15 pulses (allow for stabilization) (starting at a Dp phase), due to the design of find_values(),
         * the even indices of cross_points indicate the start of the APD phase (and end of the DP phase),
         * the odd ones mark its end and the start of the DP phase.
        */ 
        int index = 15*2-1; // Ignore the first 15 pulses (starting at a DP phase)
        int found_excitations = cross_points.size/2 - 15; // Number of useful excitations found (integer division!)
        

        for (int j = 0; j < found_excitations; j++) {
            
            if(index+2 >= cross_points.size) {
                break; // Avoid out of bounds access
            }

            DP.data[j + total_excitations] = VEC(cross_points, index + 1) - VEC(cross_points, index); // Calculate PD
            APD.data[j + total_excitations] = VEC(cross_points, index + 2) - VEC(cross_points, index + 1); // Calculate APD
            index += 2; // Move to the next pair of crossing points
        }
        total_excitations += found_excitations; // Update the total number of excitations found
        //printf("Found %d stabilized excitations for T_exc = %.2f\n", found_excitations, excitation[1]);

        // This should not be an S1-S2 method, although the ODE starts again at the end of the last pulse train.
        // An S1-S2 method would not need the waiting time for pulse stabilization.
        //initial_y[0] = MAT(result_t, 0, num_steps-1); // Update the initial voltage for the next iteration
        //initial_y[1] = MAT(result_t, 1, num_steps-1); // Update the initial fast-gate for the next iteration
        //initial_y[2] = MAT(result_t, 2, num_steps-1); // Update the initial slow-gate for the next iteration

            // Plot the results
        //Plot Alternance;
        //single_plot(&Alternance, &t_t, &y_t, "Alternance", "Time (s)", "Voltage (V)", PLOT_LINE);

        free_matrix(&result_t); // Free the matrix after use, vectors are freed too with this action.
    }
    DP.size = total_excitations; // Update the size of the vector to the number of crossing points found
    APD.size = total_excitations; // Update the size of the vector to the number of crossing points found
    
    Plot bifurcationPlot;
    single_plot(&bifurcationPlot, &DP, &APD, "Bifurcation Diagram", "DP", "APD", PLOT_SCATTER);
}

int main(int argc, char *argv[])
{
 
    double step_size = 0.05; 
    int num_steps = 30000;
    double num_points = 100; // Number of points for the bifurcation diagram
    bool plot_bifurcation_diagram = 0; // Flag to indicate bifurcation diagram plotting

    double initial_t = 0.0;
    double initial_y[] = {0.0, .9, .9};
        // param=[tv+, tv1-, tv2-, tw+, tw-, td, t0, tr, tsi, k, Vsic, Vc, Vv, J_exc]
    //double param[14] = {3.33, 9, 8, 250, 60, .395, 9, 33.33, 29, 15, .5, .13, .04, 1}; // Example parameters set 6
    double param[14] = {3.33, 15.6, 5, 350, 80, .407, 9, 34, 26.5, 15, .45, .15, .04, 1}; // Example parameters set 4
    double excitation[3]={1, 300}; // Default Periodic excitation parameters [T_exc, T_tot]
    double bifurcation[3] = {1, 300, 400}; // Bifurcation parameters [T_exc, T_tot_min, T_tot_max]
    
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
        } else if(strcmp(argv[i], "-exc") == 0 && i + 2 < argc) {
            for (int j = 0; j < 2; j++) {
                excitation[j] = atof(argv[++i]);
            }
        } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
            help_display();
            return 0;
        } else if (strcmp(argv[i], "-npt") == 0 && i + 1 < argc) {
            num_points = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-bif") == 0) {
            plot_bifurcation_diagram = 1; // Set a flag to indicate bifurcation diagram plotting
        } else if (strcmp(argv[i], "-bif_set") == 0 && i + 3 < argc) {
            for (int j = 0; j < 3; j++) {
                bifurcation[j] = atof(argv[++i]);
            }
        }

        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            help_display();
            return 1;
        }
    }
    
    // Plot the results

    Matrix result= euler_integration_multidimensional(ODE_func, step_size, num_steps, initial_t, initial_y, 3, param, excitation);

    //print_matrix(&result); // Print the matrix for debugging

    Vector t = read_matrix_row(&result, 0); // Time data is stored in the first row
    Vector y = read_matrix_row(&result, 1); // ODE Voltage values are stored in the second row

    Plot Alternance;
    single_plot(&Alternance, &t, &y, "Alternance", "Time (s)", "Voltage (V)", PLOT_LINE);

    free_matrix(&result); // Free the matrix after use, vectors are freed too with this action.

    // Plot the bifurcation diagram
    if(plot_bifurcation_diagram) {
        bifurcation_diagram(bifurcation, num_points, step_size, num_steps, initial_t, initial_y, param); // Call the bifurcation diagram function
    }
    //free_matrix(&result); // Free the result matrix

    return 0;

    }

    