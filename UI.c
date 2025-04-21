#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "include/common.h"

// --------------------------- UI & GRAPHICS ---------------------------
// Function to plot data using SDL

void plot_with_sdl(const Vector *x, const Vector *y, const double *axes) {
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
    int window_width = 800;
    int window_height = 600;
    SDL_Window *win = SDL_CreateWindow("2D Plot", 100, 100, window_width, window_height, SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    // Create an SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_SOFTWARE);
    if (!renderer) {
        fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return;
    }

    // Clear the screen
    if (SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255) != 0) {
        fprintf(stderr, "SDL_SetRenderDrawColor Error: %s\n", SDL_GetError());
    }
    if (SDL_RenderClear(renderer) != 0) {
        fprintf(stderr, "SDL_RenderClear Error: %s\n", SDL_GetError());
    }

    // Determine scaling factors and offsets to fit data within the window
    double x_min = axes[0], x_max = axes[1], y_min = axes[2], y_max = axes[3];

    double x_scale = window_width / (x_max - x_min);
    double y_scale = window_height / (y_max - y_min);

    // Draw axes
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for the axes

    int x_axis_pos = abs(y_min * y_scale); // Y=0 line
    int y_axis_pos = abs(x_min * x_scale); // X=0 line

    SDL_RenderDrawLine(renderer, 0, x_axis_pos, window_width, x_axis_pos); // X-axis
    SDL_RenderDrawLine(renderer, y_axis_pos, 0, y_axis_pos, window_height); // Y-axis

    // Draw grid and ticks
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light gray for the grid
    int htick = (int)( x_scale * (x_max - x_min) / 10 ); // Example: about 10 ticks
    int vtick = (int)( y_scale * (y_max - y_min) / 10 ); // Example: about 10 ticks

    for (int v_pos = y_axis_pos + vtick; v_pos <= window_width; v_pos += vtick) {
        SDL_RenderDrawLine(renderer, v_pos, 0, v_pos, window_height); // Right Vertical grid line
    }
    for (int h_pos = x_axis_pos + htick; h_pos <= window_height; h_pos += htick) {
        SDL_RenderDrawLine(renderer, 0, h_pos, window_width, h_pos); // Top Horizontal grid line
    }
    for (int v_pos = y_axis_pos - vtick; v_pos >= 0; v_pos -= vtick) {
        SDL_RenderDrawLine(renderer, v_pos, 0, v_pos, window_height); // Left Vertical grid line
    }
    for (int h_pos = x_axis_pos - htick; h_pos >= 0; h_pos -= htick) {
        SDL_RenderDrawLine(renderer, 0, h_pos, window_width, h_pos); // Bottom Horizontal grid line
    }

    // Draw the plot
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for the plot
    for (int i = 0; i < x->size - 1; i++) {
        int x1 = (int)((x->data[i] - x_min) * x_scale);
        int y1 = (int)((y->data[i] - y_min) * y_scale);
        int x2 = (int)((x->data[i + 1] - x_min) * x_scale);
        int y2 = (int)((y->data[i + 1] - y_min) * y_scale);
        if (SDL_RenderDrawLine(renderer, x1, y1, x2, y2) != 0) {
            fprintf(stderr, "SDL_RenderDrawLine Error: %s\n", SDL_GetError());
        }
    }

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
    // We could support fullscreen

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