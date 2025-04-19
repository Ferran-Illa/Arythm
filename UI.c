#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "include/common.h"

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
#endif // UI_HEADER_H