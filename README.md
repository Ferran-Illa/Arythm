# Arythm
Multiscale models for a first approach to the modelling of electrical impulses in the heart. These simulations are mainly focused towards the understanding of the alternance phenomenon, spiraling (tachychardia), and spiral breaking (fibrillation). The model is brought from a single-cell perspective to 1D and ultimately 2D matrixes of cells, coupled by diffusion.

## SingleCell: Work In Progress

The `SingleCell` module simulates the electrical behavior of a single cardiac cell using a system of ordinary differential equations (ODEs). It provides two main functionalities:

### 1. Alternance in a Single Cell
By solving a 3-Variable ODE system for a single cell, we may observe the alternance phenomenon when the time between excitations is short enough and the parameters are adequately tuned. The behaviour of the alternance may be seen in a bifurcation diagram, where two branches appear as the action potential splits.

#### How It Works:
- The program uses the **Euler method** to numerically integrate the ODEs governing the cell's behavior.
- The user can specify initial conditions, parameters, and excitation settings via command-line arguments.
- The resulting voltage values are plotted against time, providing insight into the cell's electrical activity.

### Command-Line Arguments

The program accepts the following command-line arguments to customize its behavior:

- `-stp <step_size>`: Specify the step size for the ODE solver (default: 0.05).
- `-nstp <num_steps>`: Specify the number of steps for the ODE solver (default: 30000).
- `-npt <num_points>`: Specify the number of points for the bifurcation diagram (default: 100).
- `-t <initial_t>`: Specify the initial time value (default: 0.0).
- `-y <V> <v> <w>`: Specify the initial values for the ODE system (default: 0.0, 0.9, 0.9).
- `-param <p1> ... <p14>`: Specify the 14 parameters for the ODE system (default: predefined values).
- `-exc <exc_time> <T_tot>`: Specify the excitation parameters (default: 1, 300).
- `-bif`: Plot the bifurcation diagram.
- `-bif_set <T_exc> <T_tot_min> <T_tot_max>`: Specify bifurcation parameters (default: 1, 300, 400).
- `-vcell`: Plot the electrical potential of a single cardiac cell over time.
- `-h, -help`: Display the help message and exit.

### Examples

#### Plotting the Potential of a Single Cell
```
./SingleCell.sh -stp 0.05 -nstp 30000 -t 0.0 -y 0.0 0.9 0.9 -param 3.33 15.6 5 350 80 0.407 9 34 26.5 15 0.45 0.15 0.04 1 -exc 1 300 -vcell
```

#### Plotting the Bifurcation Diagram
```
./SingleCell.sh -stp 0.05 -nstp 30000 -npt 100 -bif -bif_set 1 300 400
```
