#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

typedef double (*ODEFunction)(double t, double y);

typedef struct {
    int size;
    double *data;
} Vector;

typedef struct {
    int rows;
    int cols;
    double *data;
} Matrix;

// ---------------------------- VECTOR & MATRIX ---------------------------

Vector create_vector(int size) {
    Vector vec;
    vec.size = size;
    vec.data = (double *)malloc(size * sizeof(double));
    return vec;
}

Matrix create_matrix(int rows, int cols) {
    Matrix mat;
    mat.rows = rows;
    mat.cols = cols;
    mat.data = (double *)malloc(rows * cols * sizeof(double));
    return mat;
}

void free_vector(Vector *vec) {
    free(vec->data);
    vec->data = NULL;
}

void free_matrix(Matrix *mat) {
    free(mat->data);
    mat->data = NULL;
}

#define MAT(m, i, j) (m.data[(i) * (m.cols) + (j)]) // Access element at (i, j), zero-indexed!!
#define VEC(v, i) (v.data[i]) // Access element at i, zero-indexed!!

Matrix matrix_product(const Matrix *a, const Matrix *b) {
    if (a->cols != b->rows) {
        fprintf(stderr, "Matrix dimensions do not match for multiplication.\n");
        exit(EXIT_FAILURE);
    }

    Matrix result = create_matrix(a->rows, b->cols);

    for (int i = 0; i < a->rows; i++) {
        for (int j = 0; j < b->cols; j++) {
            MAT(result, i, j) = 0.0;
            for (int k = 0; k < a->cols; k++) {
                MAT(result, i, j) += MAT((*a), i, k) * MAT((*b), k, j);
            }
        }
    }

    return result;
}

// ---------------------------- ODE SOLVER ---------------------------

Matrix euler_integration(ODEFunction ode_func, double step_size, int num_steps, double initial_t, double initial_y) {
    double t = initial_t;
    double y = initial_y;
    Matrix result = create_matrix(2, num_steps); // 2 rows, for t and y

    for (int i = 0; i < num_steps; i++) {
        MAT(result, 0, i) = t;
        MAT(result, 1, i) = y;
        //printf("Step %d: t = %f, y = %f\n", i, t, y); // If we need to print all steps
        y += step_size * ode_func(t, y);
        t += step_size;
    }
    return result;
}

double expODE(double t, double y) {
    return -y; // dy/dt = -y
}

// --------------------------- UI & GRAPHICS ---------------------------
// Function to plot data using SDL

void plot_with_sdl(const Vector *x, const Vector *y) {
    if (x->size != y->size) {
        fprintf(stderr, "Vectors x and y must have the same size for plotting.\n");
        return;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return;
    }

    // Create an SDL window
    SDL_Window *win = SDL_CreateWindow("2D Plot", 100, 100, 800, 600, SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    // Create an SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return;
    }

    // Clear the screen
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255); // White background
    SDL_RenderClear(renderer);

    // Draw the plot
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for the plot
    for (int i = 0; i < x->size - 1; i++) {
        int x1 = (int)(x->data[i] * 800); // Scale and center x
        int y1 = (int)(300 - y->data[i] * 60); // Scale and invert y
        int x2 = (int)(x->data[i + 1] * 800);
        int y2 = (int)(300 - y->data[i + 1] * 60);
        SDL_RenderDrawLine(renderer, x1, y1, x2, y2);
    } // TODO: Add axes and labels; Automatically scale the plot to fit the data

    // Present the renderer
    SDL_RenderPresent(renderer);

    // Wait for the user to close the window
    SDL_Event e;
    int quit = 0;
    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                quit = 1;
            }
        }
    }

    // Clean up
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

void plot(const Vector *x, const Vector *y) {
    // Placeholder for plotting function
    printf("Plotting data...\n");
    for (int i = 0; i < x->size; i++) {
        printf("x: %f, y: %f\n", x->data[i], y->data[i]);
    }
}

void print_matrix(const Matrix *mat) {
    for (int i = 0; i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++) {
            printf("%f ", MAT((*mat), i, j));
        }
        printf("\n");
    }
}

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