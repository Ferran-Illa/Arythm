#include "include/common.h"

#ifndef ALGEBRA_H
#define ALGEBRA_H

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

Matrix copy_matrix(const Matrix *src) {
    Matrix dest = create_matrix(src->rows, src->cols);
    for (int i = 0; i < src->rows; i++) {
        for (int j = 0; j < src->cols; j++) {
            MAT(dest, i, j) = MAT(*src, i, j);
        }
    }
    return dest;
}

void free_vector(Vector *vec) {
    free(vec->data);
    vec->data = NULL;
}

void free_matrix(Matrix *mat) {
    free(mat->data);
    mat->data = NULL;
}



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

#endif // ALGEBRA_H