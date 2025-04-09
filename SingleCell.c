#include <stdio.h>

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

#define MAT(m, i, j) (m.data[(i) * (m.cols) + (j)])



void euler_integration(ODEFunction ode_func, double step_size, int num_steps, double initial_t, double initial_y) {
    double t = initial_t;
    double y = initial_y;

    for (int i = 0; i < num_steps; i++) {
        printf("Step %d: t = %f, y = %f\n", i, t, y);
        y += step_size * ode_func(t, y);
        t += step_size;
    }
}