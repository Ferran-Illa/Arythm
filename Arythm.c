#include "include/common.h"
#include "include/functions.h"
#include "include/plotting.h"

// ----------------------------- MAIN -----------------------------
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
Vector find_values(const Vector time_t , const Vector y_t, int num_excitations, int num_steps, double step_size, double threshold) {
    Vector ans= create_vector(2*num_excitations);
    bool STATE=1;
    int j = 0;
    for (int i = 0; i < num_steps; i++) {
        if (VEC(y_t, i) > threshold && STATE) {
            VEC(ans, j) = VEC(time_t, i) - step_size*(VEC(y_t, i) - threshold)/(VEC(y_t, i) - VEC(y_t, i-1)); // Interpolate the crossing point
            STATE=!STATE;
            j++;
        }
        if (VEC(y_t, i) < threshold && !STATE ) {
            VEC(ans, j) = VEC(time_t, i) - step_size*(threshold - VEC(y_t, i))/(VEC(y_t, i-1) - VEC(y_t, i));
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
    printf("  -bif                      Plot the bifurcation diagram.\n");
    printf("  -bif_set <T_exc> <T_tot_min> <T_tot_max> Specify bifurcation parameters (default: 1, 300, 400).\n");
    printf("  -cellsz <cell_size>       Specify the cell size (default: 1).\n");
    printf("  -diff <diffusion>         Specify the diffusion coefficient (default: 1).\n");
    printf("  -exc <exc_time> <T_tot>   Specify the excitation parameters (default: 1, 300).\n");
    printf("  -ex_cell <x> <y>          Specify the excited cells (default: 20, 20).\n");
    printf("  -speed <num_frames>       Specify the number of iterations per frame for the 1D plot (default: 5).\n");
    printf("  -h, -help                 Display this help message and exit.\n");
    printf("  -npt <num_points>         Specify the number of points for the bifurcation diagram (default: 100).\n");
    printf("  -nstp <num_steps>         Specify the number of steps for the ODE solver (default: 30000).\n");    
    printf("  -param <p1> ... <p14>     Specify the 14 parameters for the ODE system (default: predefined values).\n");
    printf("  -stp <step_size>          Specify the step size for the ODE solver (default: 0.05).\n");
    printf("  -t <initial_t>            Specify the initial time value (default: 0.0).\n");
    printf("  -tissue <x> <y>           Specify the tissue size (default: 100, 100).\n");
    printf("  -vcell                    Plot the single cell potential.\n");
    printf("  -y <V> <v> <w>            Specify the initial values for the ODE system (default: 0.0, 0.9, 0.9).\n");
    printf("  -1D                       Plot the 1D bifurcation diagram.\n");
    printf("  -2D                       Plot the 2D bifurcation diagram.\n");
    
    
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

void bifurcation_diagram(double bifurcation[3], int num_points, OdeFunctionParams ode_input) {

    // Extract parameters from the input structure. Beware that ode_input is not changed!
    int     nstp         = ode_input.num_steps;
    double  step_size    = ode_input.step_size;
    
    double t_tot_min = bifurcation[1];
    double t_tot_max = bifurcation[2];
    double t_tot_step = (t_tot_max - t_tot_min) / (num_points - 1); // Step size for total excitation duration

    int skip_excitations = 20; // Number of excitations to skip at start of T_exc.
    int num_excitations = 5; // Number of excitations to consider for each T_exc.

    Vector APD = create_vector(2*num_points); // Create a vector to store the APD values.
    Vector DP = create_vector(2*num_points); // Create a vector to store the DP values.

    //setting time to stabilize (skipping excitations)
    ode_input.num_steps = (int)(skip_excitations*t_tot_max / step_size - 1); // Update num_steps based on the new T_exc, allowing for 10 j.
    ode_input.excitation[0] = bifurcation[0]; // J_exc
    ode_input.excitation[1] = t_tot_max; // T_exc

    Matrix result_t = euler_integration_multidimensional(ODE_func, ode_input);

    int total_excitations = 0; // Total number of excitations found so far

    // Loop over T_exc values
    for (int i = 0; i < num_points; i++) {
        nstp = ode_input.num_steps; // Update num_steps for each iteration
        ode_input.excitation[1] = t_tot_max - i * t_tot_step; // T_exc

        ode_input.initial_y[0] = MAT(result_t, 1, nstp-1); // Update the initial voltage for the next iteration
        ode_input.initial_y[1] = MAT(result_t, 2, nstp-1); // Update the initial fast-gate for the next iteration
        ode_input.initial_y[2] = MAT(result_t, 3, nstp-1); // Update the initial slow-gate for the next iteration

        double expected_t = ode_input.initial_t + step_size * (nstp - 1); // Expected time after nstp steps
        ode_input.initial_t = MAT(result_t, 0, nstp-1); // Update the initial voltage for the next iteration

        if (ode_input.initial_t - expected_t > 0.00001 || ode_input.initial_t - expected_t < -0.00001) {
            printf("ERROR: The initial time does not match the expected value.\n");
        }
        if (result_t.rows !=4) {
            printf("ERROR: The result matrix does not have the expected number of rows.\n");
        }

        if (result_t.cols != nstp) {
            printf( "Steps considered: %.5d\n",nstp);
            printf(" Number of columns: %.5d\n",result_t.cols);
            printf("ERROR: The result matrix does not have the expected number of columns.\n");
        }

        ode_input.num_steps = (int)(num_excitations * ode_input.excitation[1] / step_size - 1); // Update num_steps based on the new T_exc, allowing for 10 j.

        // Solve the ODE system
        
        result_t = euler_integration_multidimensional(ODE_func, ode_input);

        Vector t_t = read_matrix_row(&result_t, 0); // Time data is stored in the first row
        Vector y_t = read_matrix_row(&result_t, 1); // ODE Voltage values are stored in the second row

        Vector cross_points = find_values(t_t, y_t, num_excitations, nstp, step_size, ode_input.param[11]); // Find the crossing points for the first 10 j
           
        /* 
         * Ignore the first pulse (allow for stabilization) (starting at a Dp phase), due to the design of find_values(),
         * the even indices of cross_points indicate the start of the APD phase (and end of the DP phase),
         * the odd ones mark its end and the start of the DP phase.
        */ 
        int index = 1; // Ignore the first pulse (starting at a DP phase)     
            
        for(int j = 0; j < 4; j+=2){ // number of crossings (pulses*2) to consider
            if(index + j + 2 <= cross_points.size) {        
                DP.data[total_excitations] = ode_input.excitation[1]; // Calculate PD
                APD.data[ total_excitations] = VEC(cross_points, index + j+2) - VEC(cross_points, index + j+1); // Calculate APD

                total_excitations += 1; // Update the total number of excitations found
            }

        }
        // Plot the results (debugging)
        // Plot Alternance;
        // single_plot(&Alternance, &t_t, &y_t, "Alternance", "Time (s)", "Voltage (V)", PLOT_LINE);    
    }

    free_matrix(&result_t); // Free the matrix after use, vectors are freed too with this action.

    DP.size = total_excitations; // Update the size of the vector to the number of crossing points found
    APD.size = total_excitations; // Update the size of the vector to the number of crossing points found

    Plot bifurcationPlot;
    single_plot(&bifurcationPlot, &DP, &APD, "Bifurcation Diagram", "DP", "APD", PLOT_SCATTER);
}

void parse_input(int argc, char *argv[], InputParams *input) {
    // Default values
    input -> step_size = 0.05;
    input -> num_steps = 30000;
    input -> num_points = 100;
    input -> tissue_size[0] = 100;
    input -> tissue_size[1] = 100;
    input -> excited_cells[0] = 10;
    input -> excited_cells[1] = 10;
    
    input -> plot_bifurcation_diagram = false;
    input -> plot_singlecell_potential = false;
    input -> plot_1D = false;
    input -> plot_2D = false;

    input -> initial_t = 0.0;
    input -> frame_speed = 5;

    input -> initial_y[0] = 0.0;
    input -> initial_y[1] = 0.9;
    input -> initial_y[2] = 0.9;

    // Default parameters
    double default_param[14] = {3.33, 15.6, 5, 350, 80, 0.407, 9, 34, 26.5, 15, 0.45, 0.15, 0.04, 0.1};
    memcpy(input->param, default_param, sizeof(default_param));

    // Default excitation and bifurcation parameters
    input -> excitation[0] = 2;
    input -> excitation[1] = 200;
    input -> excitation[2] = 300;

    input -> bifurcation[0] = 2.55;
    input -> bifurcation[1] = 120;
    input -> bifurcation[2] = 350;

    input -> diffusion = 1;
    input -> cell_size = 1;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-stp") == 0 && i + 1 < argc) {
            
            input->step_size = atof(argv[++i]);

        } else if (strcmp(argv[i], "-nstp") == 0 && i + 1 < argc) {
            
            input->num_steps = atoi(argv[++i]);

        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            
            input->initial_t = atof(argv[++i]);

        } else if (strcmp(argv[i], "-y") == 0 && i + 3 < argc) {
            
            for (int j = 0; j < 3; j++) {
                input->initial_y[j] = atof(argv[++i]);
            }

        } else if (strcmp(argv[i], "-param") == 0 && i + 14 < argc) {
            
            for (int j = 0; j < 14; j++) {
                input->param[j] = atof(argv[++i]);
            }

        } else if (strcmp(argv[i], "-exc") == 0 && i + 2 < argc) {
            
            for (int j = 0; j < 2; j++) {
                input->excitation[j] = atof(argv[++i]);
            }

        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "-help") == 0) {
            
            help_display();
            exit(0);

        } else if (strcmp(argv[i], "-npt") == 0 && i + 1 < argc) {
            
            input->num_points = atoi(argv[++i]);

        } else if (strcmp(argv[i], "-bif") == 0) {
            
            input->plot_bifurcation_diagram = true;

        } else if (strcmp(argv[i], "-bif_set") == 0 && i + 3 < argc) {
            
            for (int j = 0; j < 3; j++) {
                input->bifurcation[j] = atof(argv[++i]);
            }

        } else if (strcmp(argv[i], "-vcell") == 0){

            input->plot_singlecell_potential = true;

        } else if (strcmp(argv[i], "-1D") == 0){

            input->plot_1D = true;
            
        } else if (strcmp(argv[i], "-2D") == 0){

            input->plot_2D = true;
            
        } else if (strcmp(argv[i], "-tissue") == 0 && i + 2 < argc){

            for (int j = 0; j < 2; j++) {
                input->tissue_size[j] = atof(argv[++i]);
            }
            
        } else if (strcmp(argv[i], "-speed") == 0 && i + 1 < argc){

            input->frame_speed = atof(argv[++i]);
            
        } else if (strcmp(argv[i], "-cellsz") == 0 && i + 1 < argc){

            input->cell_size = atof(argv[++i]);
            
        } else if (strcmp(argv[i], "-diff") == 0 && i + 1 < argc){

            input->diffusion = atof(argv[++i]);
            
        } else if (strcmp(argv[i], "-ex_cell") == 0 && i + 2 < argc){

            for (int j = 0; j < 2; j++) {
                input->excited_cells[j] = atof(argv[++i]);
            }
            
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            help_display();
            exit(1);
        }
    }
}

int main(int argc, char *argv[])
{
    // TODO: params and param are very different, consider renaming them to avoid confusion.
    InputParams input;

    // Parse input arguments
    parse_input(argc, argv, &input);

    OdeFunctionParams ode_input = {
        .step_size  = input.step_size,
        .num_steps  = input.num_steps,
        .initial_t  = input.initial_t,
        .initial_y  = {input.initial_y[0], input.initial_y[1], input.initial_y[2]},
        .excitation = {input.excitation[0], input.excitation[1], input.excitation[2]}
    };
    memcpy(ode_input.param, input.param, sizeof(input.param)); // Copy the parameters to the ODE input
    
    // Plot the single cell potential
    if(input.plot_singlecell_potential){
        Matrix result = euler_integration_multidimensional(ODE_func, ode_input);

        Vector time = read_matrix_row(&result, 0); // Time data is stored in the first row
        Vector voltage = read_matrix_row(&result, 1); // ODE Voltage values are stored in the second row

        Plot Alternance;
        single_plot(&Alternance, &time, &voltage, "Alternance", "Time (s)", "Voltage (V)", PLOT_LINE);

        free_matrix(&result); // Free the matrix after use, vectors are freed too with this action.
    }
    
    // Plot the bifurcation diagram
    if(input.plot_bifurcation_diagram) {
        bifurcation_diagram(input.bifurcation, input.num_points, ode_input); // Call the bifurcation diagram function
    }
    
    // Plot the 1D bifurcation diagram
    if(input.plot_1D){
        // Initialization
        // What was once an array is now a "matrix", three vectors.
        int rows = input.tissue_size[0];
        int cols = input.tissue_size[1];

        double time = 0.0;

        Vector M_voltage = create_vector(input.tissue_size[0]); 
        Vector M_vgate   = create_vector(input.tissue_size[0]);
        Vector M_wgate   = create_vector(input.tissue_size[0]);
        Vector M_pos     = create_vector(input.tissue_size[0]); // Index vector for the cells

        // Set the initial conditions for each grid point
        for(int i = 0; i < input.tissue_size[0]; i++){
            M_voltage.data[i] = input.initial_y[0];
            M_vgate.data[i]   = input.initial_y[1];
            M_wgate.data[i]   = input.initial_y[2];
            M_pos.data[i]     = i*input.cell_size; // Set the position of each cell
        }

        // Time Evolution
        DiffusionData diffusion_config = {
            .rows = rows,
            .cols = cols,
            .time = time,
            .M_voltage = &M_voltage,
            .M_vgate   = &M_vgate,
            .M_wgate   = &M_wgate,
            .diffusion = input.diffusion,
            .cell_size = input.cell_size,
            .excited_cells = input.excited_cells[0]
        };

        Plot diffusion_plot;
        plot_init(&diffusion_plot); // Initialize the plot

        strcpy(diffusion_plot.title, "Diffusion 1D");
        strcpy(diffusion_plot.x_label, "Distance (x)");
        strcpy(diffusion_plot.y_label, "Voltage (V)");

        // Add series to the plot
        plot_add_series(&diffusion_plot, &M_pos, &M_voltage, "Diffusion in 1D", (Color){0, 0, 0, 255}, LINE_SOLID, MARKER_CIRCLE, 1, 2, PLOT_LINE);
        plot_config_video(&diffusion_plot, true, diffusion1D, &diffusion_config, &ode_input, input.frame_speed); // Dynamic plot

        PlotError error = plot_show(&diffusion_plot);
        if (error != PLOT_SUCCESS) {
            fprintf(stderr, "Error showing plot: %d\n", error);
        return -1;
        }

        // Clean up
        plot_cleanup(&diffusion_plot);
        /*
        int speed = 10;
        for( int f = 0; f < input.frames; f++ ){

            diffusion1D(ode_input, diffusion_config); // Diffusion step
            diffusion_config.time += ode_input.step_size;
            
            if(speed == 10){
                // Plot current frame
            }
            speed += 1;
        }
        */
    }

    // Plot the 2D bifurcation diagram
    if(input.plot_2D){
        printf("Work In Progress");
    }
    return 0;

    }

    