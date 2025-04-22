#ifndef UI_HEADER_H
#define UI_HEADER_H

#include "include/common.h"

// --------------------------- UI & GRAPHICS ---------------------------
// Function to plot data using SDL

void axis_digits(char *label, double label_num, int significant_digits) {
    // Format the label with the specified number of significant digits
    snprintf(label, significant_digits, "%.*g", significant_digits, label_num);
}

void Render_Background(SDL_Renderer *renderer, int plot_width, int plot_height, const double *axes, TTF_Font *font, double x_tick, double y_tick) {
    
    double x_min = axes[0], x_max = axes[1], y_min = axes[2], y_max = axes[3];

    double x_scale = plot_width / (x_max - x_min);
    double y_scale = plot_height / (y_max - y_min);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black for the axes

    int x_axis_pos = (int)(y_max * y_scale); // Y=0 line
    int y_axis_pos = (int)(-x_min * x_scale); // X=0 line

    SDL_RenderDrawLine(renderer, 0, x_axis_pos, plot_width, x_axis_pos); // X-axis
    SDL_RenderDrawLine(renderer, y_axis_pos, 0, y_axis_pos, plot_height); // Y-axis

    // Draw grid and ticks
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255); // Light gray for the grid

    int vtick = (int)( x_scale * x_tick ); 
    int htick = (int)( y_scale * y_tick ); 

    int tick_label_xoffset = 5; // x offset for grid labels (in pixels)
    int tick_label_yoffset = -15; // y offset for grid labels (in pixels)
    
    volatile double label_num = x_tick;
    int significant_digits = 4; // Number of significant digits for the labels
    char label[significant_digits + 4]; // Buffer for the label text

    // Draw vertical grid lines and numbers
    // Warning: Label position does not account for window size, they may appear out of bounds.
    for (int v_pos = y_axis_pos + vtick; v_pos <= plot_width; v_pos += vtick, label_num += x_tick) {
        
        SDL_RenderDrawLine(renderer, v_pos, 0, v_pos, plot_height); // Right Vertical grid line
        
        axis_digits(label, label_num, significant_digits); // Format the label

        SDL_Surface *surface = TTF_RenderText_Solid(font, label, (SDL_Color){0, 0, 0, 255}); // Black Text
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dest = {v_pos + tick_label_xoffset, x_axis_pos + tick_label_yoffset, surface->w, surface->h}; // Adjust position of the label
        
        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

    } 
    label_num = -x_tick;
    for (int v_pos = y_axis_pos - vtick; v_pos >= 0; v_pos -= vtick, label_num -= x_tick) {
        
        SDL_RenderDrawLine(renderer, v_pos, 0, v_pos, plot_height); // Left Vertical grid line
        
        axis_digits(label, label_num, significant_digits); // Format the label

        SDL_Surface *surface = TTF_RenderText_Solid(font, label, (SDL_Color){0, 0, 0, 255});
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dest = {v_pos + tick_label_xoffset, x_axis_pos + tick_label_yoffset, surface->w, surface->h};

        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);

    } 
    

    // Draw horizontal grid lines and numbers
    label_num = -y_tick;
    for (int h_pos = x_axis_pos + htick; h_pos <= plot_height; h_pos += htick, label_num -= y_tick) {
        
        SDL_RenderDrawLine(renderer, 0, h_pos, plot_width, h_pos); // Top Horizontal grid line
        
        axis_digits(label, label_num, significant_digits); // Format the label

        SDL_Surface *surface = TTF_RenderText_Solid(font, label, (SDL_Color){0, 0, 0, 255});
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dest = {y_axis_pos + tick_label_xoffset, h_pos + tick_label_yoffset, surface->w, surface->h};

        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    } 
    label_num = y_tick;
    for (int h_pos = x_axis_pos - htick; h_pos >= 0; h_pos -= htick, label_num += y_tick) {
        
        SDL_RenderDrawLine(renderer, 0, h_pos, plot_width, h_pos); // Bottom Horizontal grid line
        
        axis_digits(label, label_num, significant_digits); // Format the label

        SDL_Surface *surface = TTF_RenderText_Solid(font, label, (SDL_Color){0, 0, 0, 255});
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);

        SDL_Rect dest = {y_axis_pos + tick_label_xoffset, h_pos + tick_label_yoffset, surface->w, surface->h};

        SDL_RenderCopy(renderer, texture, NULL, &dest);
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }

}

void Render_Data(SDL_Renderer *renderer, int plot_width, int plot_height, const Vector *x, const Vector *y, const double *axes) {
    // Function to render data points on the plot

    // Determine scaling factors and offsets to fit data within the window
    double x_min = axes[0], x_max = axes[1], y_min = axes[2], y_max = axes[3];

    double x_scale = plot_width / (x_max - x_min);
    double y_scale = plot_height / (y_max - y_min);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue for the plot
    for (int i = 0; i < x->size - 1; i++) {
        int x1 = (int)((x->data[i] - x_min) * x_scale);
        int y1 = plot_height - (int)((y->data[i] - y_min) * y_scale);
        int x2 = (int)((x->data[i + 1] - x_min) * x_scale);
        int y2 = plot_height - (int)((y->data[i + 1] - y_min) * y_scale);
        if (SDL_RenderDrawLine(renderer, x1, y1, x2, y2) != 0) {
            fprintf(stderr, "SDL_RenderDrawLine Error: %s\n", SDL_GetError());
        }
    }
   
}
void plot_with_sdl(const Vector *x, const Vector *y, const double *axes, const double x_tick, const double y_tick) {
    if (x->size != y->size) {
        fprintf(stderr, "Vectors x and y must have the same size for plotting.\n");
        return;
    }

    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return;
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return;
    }

    TTF_Font *font = TTF_OpenFont("/usr/share/fonts/truetype/msttcorefonts/times.ttf", 14); // Times New Roman size 14
    if (!font) {
        fprintf(stderr, "TTF_OpenFont Error: %s\n", TTF_GetError());
        TTF_Quit();
        SDL_Quit();
        return;
    }

    // Create an SDL window

    int plot_width = 800;
    int plot_height = 600;

    // Set margins for the plot
    int margin_left = 50;
    int margin_right = 50;
    int margin_top = 0;
    int margin_bottom = 10;

    int window_width = plot_width + margin_left + margin_right; // This is gonna need a lot of changes
    int window_height = plot_height + margin_top + margin_bottom;

    // Create an SDL window
    SDL_Window *win = SDL_CreateWindow("2D Plot", 100, 100, plot_width, window_height, SDL_WINDOW_SHOWN);
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
 
    // Draw the background and axes
    Render_Background(renderer, plot_width, plot_height, axes, font, x_tick, y_tick);

    // Draw the data points
    Render_Data(renderer, plot_width, plot_height, x, y, axes);
    
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
    TTF_CloseFont(font);
    TTF_Quit();
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