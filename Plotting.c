#include "include/common.h"
#include "include/plotting.h"
#include "include/functions.h"

#ifndef PLOTTING_C
#define PLOTTING_C

// ADDED: Function to check if a point is inside a rectangle
bool point_in_rect(int x, int y, Rect rect) {
    return x >= rect.x && x < rect.x + rect.width && 
           y >= rect.y && y < rect.y + rect.height;
}

// Function to initialize a plot
PlotError plot_init(Plot* plot) {
    if (plot == NULL) {
        return PLOT_ERROR_INVALID_DATA;
    }
    
    // Initialize plot with default values
    strcpy(plot->title, "Plot");
    strcpy(plot->x_label, "X-Axis");
    strcpy(plot->y_label, "Y-Axis");
    
    plot->x_range.min = 0;
    plot->x_range.max = 10;
    plot->x_range.scale_type = SCALE_LINEAR;
    
    plot->y_range.min = 0;
    plot->y_range.max = 10;
    plot->y_range.scale_type = SCALE_LINEAR;
    
    plot->series_count = 0;
    plot->show_grid = true;
    plot->show_legend = true;

    plot->IsPaused = false; // Initialize non-paused state
    
    // Default colors
    plot->background_color = (Color){255, 255, 255, 255}; // White
    plot->grid_color = (Color){200, 200, 200, 255}; // Light gray
    plot->text_color = (Color){0, 0, 0, 255}; // Black
    plot->axis_color = (Color){0, 0, 0, 255}; // Black
    
    // Default zoom and pan
    plot->zoom_factor = 1.0;
    plot->pan_x = 0.0;
    plot->pan_y = 0.0;
    plot->auto_scale = true;
    
    // Initialize fullscreen properties
    plot->fullscreen = false;
    plot->window_width = DEFAULT_WINDOW_WIDTH;
    plot->window_height = DEFAULT_WINDOW_HEIGHT;
    
    // MODIFIED: Initialize margins with better defaults
    plot->margin_left = DEFAULT_MARGIN * 1.5;   // Extra space for y-axis labels
    plot->margin_right = DEFAULT_MARGIN;
    plot->margin_top = DEFAULT_MARGIN;
    plot->margin_bottom = DEFAULT_MARGIN * 1.5; // Extra space for x-axis labels
    
    // Initialize layout rectangles
    plot->plot_area = (Rect){
        plot->margin_left, 
        plot->margin_top, 
        plot->window_width - plot->margin_left - plot->margin_right,
        plot->window_height - plot->margin_top - plot->margin_bottom
    };
    
    // Other areas will be calculated during rendering
    
    return PLOT_SUCCESS;
}

PlotError plot_config_video(Plot* plot, bool dynamic_plot, 
        DiffVideo diff_video_generator, DiffusionData* diffusion_data, 
        OdeFunctionParams* ode_input, int frame_speed) 
{
    if (plot == NULL || diffusion_data == NULL || ode_input == NULL) {
        return PLOT_ERROR_INVALID_DATA;
    }
    
    // Set dynamic plot properties
    plot->series[0].dynamic_plot = dynamic_plot;
    plot->series[0].diff_video_generator = diff_video_generator;
    plot->series[0].diffusion_data = diffusion_data;
    plot->series[0].ode_input = ode_input;
    plot->series[0].frame_speed = frame_speed;
    
    return PLOT_SUCCESS;

}
// Function to add a data series to a plot
PlotError plot_add_series(Plot* plot, Vector* x_vec, Vector* y_vec, 
        const char* label, Color color, LineStyle line_style, MarkerType marker_type,
        int line_width, int marker_size, PlotType plot_type) 
{

    // Check for valid input
    if (x_vec == NULL || y_vec == NULL) {
    return PLOT_ERROR_INVALID_VECTOR;
    }
    if (x_vec->size != y_vec->size) {
    return PLOT_ERROR_SIZE_MISMATCH;
    }

    const double * x_data = x_vec->data;
    const double * y_data = y_vec->data;
    int data_length = x_vec->size;    
    
    if (plot == NULL || x_data == NULL || y_data == NULL || data_length <= 0) {
        return PLOT_ERROR_INVALID_DATA;
    }
    
    if (plot->series_count >= MAX_DATA_SERIES) {
        return PLOT_ERROR_MAX_SERIES;
    }
    
    DataSeries* series = &plot->series[plot->series_count];
    
    // Allocate memory for data
    series->x_data = (double*)malloc(data_length * sizeof(double));
    series->y_data = (double*)malloc(data_length * sizeof(double));
    
    if (series->x_data == NULL || series->y_data == NULL) {
        free(series->x_data);
        free(series->y_data);
        return PLOT_ERROR_INVALID_DATA;
    }
    
    // Copy data
    memcpy(series->x_data, x_data, data_length * sizeof(double));
    memcpy(series->y_data, y_data, data_length * sizeof(double));
    series->data_length = data_length;
    
    // Set other properties
    strncpy(series->label, label, MAX_LABEL_LENGTH - 1);
    series->label[MAX_LABEL_LENGTH - 1] = '\0';
    series->color = color;
    series->line_style = line_style;
    series->marker_type = marker_type;
    series->line_width = line_width;
    series->marker_size = marker_size;
    series->plot_type = plot_type;
    series->visible = true;

    series->diff_video_generator = NULL;
    series->diffusion_data = NULL;
    series->ode_input = NULL;
    series->frame_speed = 10;
    series->dynamic_plot = false;
    
    plot->series_count++;
    
    return PLOT_SUCCESS;
}
// Function to find the min and max values in an array
Range find_range(const double* data, int length, ScaleType scale_type) {
    Range range = {data[0], data[0], scale_type};
    
    // For logarithmic scale, ensure all values are positive
    if (scale_type == SCALE_LOG) {
        range.min = DBL_MAX;
        for (int i = 0; i < length; i++) {
            if (data[i] > 0 && data[i] < range.min) {
                range.min = data[i];
            }
            if (data[i] > range.max) {
                range.max = data[i];
            }
        }
    } else {
        for (int i = 1; i < length; i++) {
            if (data[i] < range.min) range.min = data[i];
            if (data[i] > range.max) range.max = data[i];
        }
    }
    
    // Add a small margin to the range
    double margin = (range.max - range.min) * 0.05;
    if (margin == 0) margin = 0.5; // In case min == max
    
    range.min -= margin;
    range.max += margin;
    
    // For logarithmic scale, ensure min is positive
    if (scale_type == SCALE_LOG && range.min <= 0) {
        range.min = range.max / 1000.0;
        if (range.min <= 0) range.min = 0.01;
    }
    
    return range;
}

// Function to auto-scale the plot based on all data series
void plot_auto_scale(Plot* plot) {
    if (plot->series_count == 0) return;
    
    // Initialize with first point of first series
    double x_min = plot->series[0].x_data[0];
    double x_max = plot->series[0].x_data[0];
    double y_min = plot->series[0].y_data[0];
    double y_max = plot->series[0].y_data[0];
    
    // Find global min and max for x and y
    for (int s = 0; s < plot->series_count; s++) {
        DataSeries* series = &plot->series[s];
        for (int i = 0; i < series->data_length; i++) {
            // For logarithmic scale, only consider positive values
            if (plot->x_range.scale_type == SCALE_LOG) {
                if (series->x_data[i] > 0 && (series->x_data[i] < x_min || x_min <= 0)) {
                    x_min = series->x_data[i];
                }
            } else {
                if (series->x_data[i] < x_min) x_min = series->x_data[i];
            }
            
            if (series->x_data[i] > x_max) x_max = series->x_data[i];
            
            if (plot->y_range.scale_type == SCALE_LOG) {
                if (series->y_data[i] > 0 && (series->y_data[i] < y_min || y_min <= 0)) {
                    y_min = series->y_data[i];
                }
            } else {
                if (series->y_data[i] < y_min) y_min = series->y_data[i];
            }
            
            if (series->y_data[i] > y_max) y_max = series->y_data[i];
        }
    }
    
    // Add margins
    double x_margin = (x_max - x_min) * 0.05;
    double y_margin = (y_max - y_min) * 0.05;
    
    if (x_margin == 0) x_margin = 0.5;
    if (y_margin == 0) y_margin = 0.5;
    
    plot->x_range.min = x_min - x_margin;
    plot->x_range.max = x_max + x_margin;
    plot->y_range.min = y_min - y_margin;
    plot->y_range.max = y_max + y_margin;
    
    // For logarithmic scales, ensure min values are positive
    if (plot->x_range.scale_type == SCALE_LOG && plot->x_range.min <= 0) {
        plot->x_range.min = plot->x_range.max / 1000.0;
        if (plot->x_range.min <= 0) plot->x_range.min = 0.01;
    }
    
    if (plot->y_range.scale_type == SCALE_LOG && plot->y_range.min <= 0) {
        plot->y_range.min = plot->y_range.max / 1000.0;
        if (plot->y_range.min <= 0) plot->y_range.min = 0.01;
    }
}

// Function to map a value from one range to another, considering scale type
double map_value(double value, double in_min, double in_max, double out_min, double out_max, ScaleType scale_type) {
    if (scale_type == SCALE_LOG) {
        // Ensure positive values for logarithmic scale
        if (value <= 0) value = in_min;
        if (in_min <= 0) in_min = 0.01;
        
        // Map in log space
        double log_value = log10(value);
        double log_in_min = log10(in_min);
        double log_in_max = log10(in_max);
        
        return (log_value - log_in_min) * (out_max - out_min) / (log_in_max - log_in_min) + out_min;
    } else {
        // Linear mapping
        return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }
}

// Function to draw a marker
void draw_marker(SDL_Renderer* renderer, int x, int y, MarkerType marker_type, int size, Color color, Rect clip_rect) {
    // MODIFIED: Skip drawing if outside the clip rectangle
    if (!point_in_rect(x, y, clip_rect)) {
        return;
    }
    
    switch (marker_type) {
        case MARKER_CIRCLE:
            filledCircleRGBA(renderer, x, y, size, color.r, color.g, color.b, color.a);
            break;
            
        case MARKER_SQUARE:
            boxRGBA(renderer, x - size, y - size, x + size, y + size, 
                   color.r, color.g, color.b, color.a);
            break;
            
        case MARKER_TRIANGLE: {
            Sint16 vx[3] = {x, x - size, x + size};
            Sint16 vy[3] = {y - size, y + size, y + size};
            filledPolygonRGBA(renderer, vx, vy, 3, color.r, color.g, color.b, color.a);
            break;
        }
            
        case MARKER_DIAMOND: {
            Sint16 vx[4] = {x, x + size, x, x - size};
            Sint16 vy[4] = {y - size, y, y + size, y};
            filledPolygonRGBA(renderer, vx, vy, 4, color.r, color.g, color.b, color.a);
            break;
        }
            
        case MARKER_CROSS:
            lineRGBA(renderer, x - size, y - size, x + size, y + size, 
                    color.r, color.g, color.b, color.a);
            lineRGBA(renderer, x - size, y + size, x + size, y - size, 
                    color.r, color.g, color.b, color.a);
            break;
            
        case MARKER_PLUS:
            lineRGBA(renderer, x - size, y, x + size, y, 
                    color.r, color.g, color.b, color.a);
            lineRGBA(renderer, x, y - size, x, y + size, 
                    color.r, color.g, color.b, color.a);
            break;
            
        case MARKER_NONE:
        default:
            // No marker
            break;
    }
}

// ADDED: Function to clip a line to a rectangle using Cohen-Sutherland algorithm
#define INSIDE 0 // 0000
#define LEFT 1   // 0001
#define RIGHT 2  // 0010
#define BOTTOM 4 // 0100
#define TOP 8    // 1000

// Compute the bit code for a point (x, y) using the clip rectangle
int compute_code(int x, int y, Rect clip_rect) {
    int code = INSIDE;
    
    if (x < clip_rect.x)           // to the left of clip window
        code |= LEFT;
    else if (x >= clip_rect.x + clip_rect.width)  // to the right of clip window
        code |= RIGHT;
    if (y < clip_rect.y)           // below the clip window
        code |= BOTTOM;
    else if (y >= clip_rect.y + clip_rect.height) // above the clip window
        code |= TOP;
    
    return code;
}

// Cohen-Sutherland line clipping algorithm
bool clip_line(int* x1, int* y1, int* x2, int* y2, Rect clip_rect) {
    // Compute region codes for P1, P2
    int code1 = compute_code(*x1, *y1, clip_rect);
    int code2 = compute_code(*x2, *y2, clip_rect);
    bool accept = false;
    
    while (true) {
        // Both endpoints inside the clip window - trivially accept
        if ((code1 | code2) == 0) {
            accept = true;
            break;
        }
        // Both endpoints outside the clip window in same region - trivially reject
        else if ((code1 & code2) != 0) {
            break;
        }
        // Some segment of the line may be inside the clip window - divide the line
        else {
            // Pick an endpoint outside the clip window
            int code_out = code1 != 0 ? code1 : code2;
            
            int x, y;
            
            // Find intersection point
            if (code_out & TOP) {
                // Point is above the clip window
                x = *x1 + (*x2 - *x1) * (clip_rect.y + clip_rect.height - 1 - *y1) / (*y2 - *y1);
                y = clip_rect.y + clip_rect.height - 1;
            } else if (code_out & BOTTOM) {
                // Point is below the clip window
                x = *x1 + (*x2 - *x1) * (clip_rect.y - *y1) / (*y2 - *y1);
                y = clip_rect.y;
            } else if (code_out & RIGHT) {
                // Point is to the right of clip window
                y = *y1 + (*y2 - *y1) * (clip_rect.x + clip_rect.width - 1 - *x1) / (*x2 - *x1);
                x = clip_rect.x + clip_rect.width - 1;
            } else if (code_out & LEFT) {
                // Point is to the left of clip window
                y = *y1 + (*y2 - *y1) * (clip_rect.x - *x1) / (*x2 - *x1);
                x = clip_rect.x;
            }
            
            // Replace the outside point with the intersection point
            if (code_out == code1) {
                *x1 = x;
                *y1 = y;
                code1 = compute_code(x, y, clip_rect);
            } else {
                *x2 = x;
                *y2 = y;
                code2 = compute_code(x, y, clip_rect);
            }
        }
    }
    
    return accept;
}

// Function to draw a line with the specified style
void draw_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, LineStyle style, int width, Color color, Rect clip_rect) {
    // MODIFIED: Clip the line to the plot area
    if (!clip_line(&x1, &y1, &x2, &y2, clip_rect)) {
        return; // Line is completely outside the clip rectangle
    }
    
    switch (style) {
        case LINE_SOLID:
            // For thicker lines
            if (width > 1) {
                thickLineRGBA(renderer, x1, y1, x2, y2, width, 
                             color.r, color.g, color.b, color.a);
            } else {
                lineRGBA(renderer, x1, y1, x2, y2, 
                        color.r, color.g, color.b, color.a);
            }
            break;
            
        case LINE_DASHED: {
            // Calculate line length and angle
            double dx = x2 - x1;
            double dy = y2 - y1;
            double length = sqrt(dx * dx + dy * dy);
            double angle = atan2(dy, dx);
            
            // Draw dashed line
            double dash_length = 8.0;
            double gap_length = 4.0;
            double pos = 0.0;
            
            while (pos < length) {
                double end_pos = pos + dash_length;
                if (end_pos > length) end_pos = length;
                
                int dash_x1 = x1 + (int)(pos * cos(angle));
                int dash_y1 = y1 + (int)(pos * sin(angle));
                int dash_x2 = x1 + (int)(end_pos * cos(angle));
                int dash_y2 = y1 + (int)(end_pos * sin(angle));
                
                if (width > 1) {
                    thickLineRGBA(renderer, dash_x1, dash_y1, dash_x2, dash_y2, width, 
                                 color.r, color.g, color.b, color.a);
                } else {
                    lineRGBA(renderer, dash_x1, dash_y1, dash_x2, dash_y2, 
                            color.r, color.g, color.b, color.a);
                }
                
                pos = end_pos + gap_length;
            }
            break;
        }
            
        case LINE_DOTTED: {
            // Calculate line length and angle
            double dx = x2 - x1;
            double dy = y2 - y1;
            double length = sqrt(dx * dx + dy * dy);
            double angle = atan2(dy, dx);
            
            // Draw dotted line
            double dot_spacing = 4.0;
            double pos = 0.0;
            
            while (pos < length) {
                int dot_x = x1 + (int)(pos * cos(angle));
                int dot_y = y1 + (int)(pos * sin(angle));
                
                // Only draw dots inside the clip rectangle
                if (point_in_rect(dot_x, dot_y, clip_rect)) {
                    filledCircleRGBA(renderer, dot_x, dot_y, width/2 + 1, 
                                    color.r, color.g, color.b, color.a);
                }
                
                pos += dot_spacing;
            }
            break;
        }
            
        case LINE_DASH_DOT: {
            // Calculate line length and angle
            double dx = x2 - x1;
            double dy = y2 - y1;
            double length = sqrt(dx * dx + dy * dy);
            double angle = atan2(dy, dx);
            
            // Draw dash-dot line
            double dash_length = 8.0;
            double gap_length = 4.0;
            double dot_length = 1.0;
            double pos = 0.0;
            
            while (pos < length) {
                // Draw dash
                double end_pos = pos + dash_length;
                if (end_pos > length) end_pos = length;
                
                int dash_x1 = x1 + (int)(pos * cos(angle));
                int dash_y1 = y1 + (int)(pos * sin(angle));
                int dash_x2 = x1 + (int)(end_pos * cos(angle));
                int dash_y2 = y1 + (int)(end_pos * sin(angle));
                
                // Clip the dash line segment
                if (clip_line(&dash_x1, &dash_y1, &dash_x2, &dash_y2, clip_rect)) {
                    if (width > 1) {
                        thickLineRGBA(renderer, dash_x1, dash_y1, dash_x2, dash_y2, width, 
                                     color.r, color.g, color.b, color.a);
                    } else {
                        lineRGBA(renderer, dash_x1, dash_y1, dash_x2, dash_y2, 
                                color.r, color.g, color.b, color.a);
                    }
                }
                
                pos = end_pos + gap_length;
                
                // Draw dot
                if (pos < length) {
                    int dot_x = x1 + (int)(pos * cos(angle));
                    int dot_y = y1 + (int)(pos * sin(angle));
                    
                    // Only draw dot if inside clip rectangle
                    if (point_in_rect(dot_x, dot_y, clip_rect)) {
                        filledCircleRGBA(renderer, dot_x, dot_y, width/2 + 1, 
                                        color.r, color.g, color.b, color.a);
                    }
                    
                    pos += dot_length + gap_length;
                }
            }
            break;
        }
    }
}

// MODIFIED: Function to get text dimensions
void get_text_dimensions(TTF_Font* font, const char* text, int* width, int* height) {
    if (font == NULL || text == NULL) {
        *width = 0;
        *height = 0;
        return;
    }
    
    TTF_SizeText(font, text, width, height);
}

// Corrected function declaration for render_text
int render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, Color color, bool center) {
    SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, sdl_color);
    
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        
        if (texture) {
            SDL_Rect rect;
            rect.x = center ? x - surface->w / 2 : x;
            rect.y = center ? y - surface->h / 2 : y;
            rect.w = surface->w;
            rect.h = surface->h;
            
            SDL_RenderCopy(renderer, texture, NULL, &rect);
            SDL_DestroyTexture(texture);
        }
        
        SDL_FreeSurface(surface);
    }
    return 0;
}

// Rendering Rotated Text (Y-Label)
void render_rotated_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, double angle, Color color) {
    SDL_Color sdl_color = {color.r, color.g, color.b, color.a};
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, sdl_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_Rect dest_rect = {x, y, surface->w, surface->h};
            SDL_Point center = {surface->w / 2, surface->h / 2}; // Rotate around the center
            SDL_RenderCopyEx(renderer, texture, NULL, &dest_rect, angle, &center, SDL_FLIP_NONE);
            SDL_DestroyTexture(texture);
        }
        SDL_FreeSurface(surface);
    }
}
// MODIFIED: Function to calculate legend dimensions
void calculate_legend_dimensions(Plot* plot, TTF_Font* font, int* width, int* height) {
    int max_label_width = 0;
    int line_length = 20;
    int item_height = 25;
    int padding = 20;
    
    for (int i = 0; i < plot->series_count; i++) {
        int label_width, label_height;
        get_text_dimensions(font, plot->series[i].label, &label_width, &label_height);
        if (label_width > max_label_width) {
            max_label_width = label_width;
        }
    }
    
    *width = line_length + max_label_width + padding;
    *height = plot->series_count * item_height + padding;
}

// Corrected function declaration for draw_legend
int draw_legend(SDL_Renderer* renderer, TTF_Font* font, Plot* plot) {
    if (!plot->show_legend || plot->series_count == 0) return 0;
    
    int legend_width, legend_height;
    calculate_legend_dimensions(plot, font, &legend_width, &legend_height);
    
    // Calculate legend position - keep it inside the plot area
    int legend_x = plot->plot_area.x + plot->plot_area.width - legend_width - 10;
    int legend_y = plot->plot_area.y + 10;
    
    // Ensure legend stays within plot area
    if (legend_x < plot->plot_area.x + 10) {
        legend_x = plot->plot_area.x + 10;
    }
    
    // Update legend area in the plot structure
    plot->legend_area = (Rect){legend_x, legend_y, legend_width, legend_height};
    
    // Draw legend background
    boxRGBA(renderer, legend_x, legend_y, 
           legend_x + legend_width, legend_y + legend_height, 
           240, 240, 240, 200);
    
    rectangleRGBA(renderer, legend_x, legend_y, 
                 legend_x + legend_width, legend_y + legend_height, 
                 100, 100, 100, 255);
    
    int line_length = 20;
    int item_height = 25;
    int padding = 10;
    
    // Draw legend items
    for (int i = 0; i < plot->series_count; i++) {
        DataSeries* series = &plot->series[i];
        int y = legend_y + padding + i * item_height;
        
        // Draw line/marker sample
        if (series->plot_type == PLOT_LINE || series->plot_type == PLOT_SCATTER || series->plot_type == PLOT_STEM) {
            if (series->plot_type != PLOT_SCATTER ) { // && series->line_style != MARKER_NONE // Cut out
                draw_line(renderer, legend_x + padding, y + item_height/2, 
                         legend_x + padding + line_length, y + item_height/2, 
                         series->line_style, series->line_width, series->color, plot->legend_area);
            }
            
            if (series->marker_type != MARKER_NONE) {
                draw_marker(renderer, legend_x + padding + line_length/2, y + item_height/2, 
                           series->marker_type, series->marker_size, series->color, plot->legend_area);
            }
        } else if (series->plot_type == PLOT_BAR) {
            boxRGBA(renderer, legend_x + padding, y + item_height/4, 
                   legend_x + padding + line_length, y + 3*item_height/4, 
                   series->color.r, series->color.g, series->color.b, series->color.a);
        }
        
        // Draw label
        render_text(renderer, font, series->label, 
                   legend_x + padding + line_length + 5, y + item_height/2 - 8, 
                   plot->text_color, false);
    }
    return 0;
}

// ADDED: Function to calculate layout based on window size and text dimensions
void calculate_layout(Plot* plot, TTF_Font* font, TTF_Font* title_font) {
    // Calculate margins based on label sizes
    int title_width, title_height;
    int x_label_width, x_label_height;
    int y_label_width, y_label_height;
    int max_y_tick_width = 0;
    int max_x_tick_height = 0;
    
    // Get title dimensions
    get_text_dimensions(title_font, plot->title, &title_width, &title_height);
    
    // Get axis label dimensions
    get_text_dimensions(font, plot->x_label, &x_label_width, &x_label_height);
    get_text_dimensions(font, plot->y_label, &y_label_width, &y_label_height);
    
    // Estimate max tick label dimensions
    char tick_label[MAX_LABEL_LENGTH];
    for (int i = 0; i <= GRID_LINES; i++) {
        // X-axis tick
        snprintf(tick_label, MAX_LABEL_LENGTH, "%.2g", 
                plot->x_range.min + (plot->x_range.max - plot->x_range.min) * i / GRID_LINES);
        int width, height;
        get_text_dimensions(font, tick_label, &width, &height);
        if (height > max_x_tick_height) max_x_tick_height = height;
        
        // Y-axis tick
        snprintf(tick_label, MAX_LABEL_LENGTH, "%.2g", 
                plot->y_range.min + (plot->y_range.max - plot->y_range.min) * i / GRID_LINES);
        get_text_dimensions(font, tick_label, &width, &height);
        if (width > max_y_tick_width) max_y_tick_width = width;
    }
    
    // Calculate margins with padding
    int padding = 10;
    plot->margin_top = title_height + padding * 2;
    plot->margin_bottom = x_label_height + max_x_tick_height + padding * 2;
    plot->margin_left = y_label_height + max_y_tick_width + padding * 2;
    plot->margin_right = padding * 2;
    
    // Ensure minimum margins
    if (plot->margin_top < DEFAULT_MARGIN) plot->margin_top = DEFAULT_MARGIN;
    if (plot->margin_bottom < DEFAULT_MARGIN) plot->margin_bottom = DEFAULT_MARGIN;
    if (plot->margin_left < DEFAULT_MARGIN) plot->margin_left = DEFAULT_MARGIN;
    if (plot->margin_right < DEFAULT_MARGIN) plot->margin_right = DEFAULT_MARGIN;
    
    // Calculate plot area
    plot->plot_area = (Rect){
        plot->margin_left, 
        plot->margin_top, 
        plot->window_width - plot->margin_left - plot->margin_right,
        plot->window_height - plot->margin_top - plot->margin_bottom
    };
    
    // Calculate other areas
    plot->title_area = (Rect){
        0, 
        padding, 
        plot->window_width,
        title_height
    };
    
    plot->x_label_area = (Rect){
        plot->margin_left, 
        plot->window_height - plot->margin_bottom + 2*padding + max_x_tick_height, 
        plot->plot_area.width,
        x_label_height
    };
    
    plot->y_label_area = (Rect){
        -padding, // God knows why this is needed to correctly position the label
        plot->margin_top, 
        y_label_width,
        plot->plot_area.height
    };
}

// Function to toggle fullscreen mode
void toggle_fullscreen(SDL_Window* window, Plot* plot) {
    plot->fullscreen = !plot->fullscreen;
    
    if (plot->fullscreen) {
        // Save current window size before going fullscreen
        SDL_GetWindowSize(window, &plot->window_width, &plot->window_height);
        
        // Switch to fullscreen
        SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        
        // Get the new window size
        SDL_GetWindowSize(window, &plot->window_width, &plot->window_height);
    } else {
        // Switch back to windowed mode
        SDL_SetWindowFullscreen(window, 0);
        
        // Restore window size
        SDL_SetWindowSize(window, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
        plot->window_width = DEFAULT_WINDOW_WIDTH;
        plot->window_height = DEFAULT_WINDOW_HEIGHT;
    }
    
    printf("Window size: %d x %d\n", plot->window_width, plot->window_height);
}

// Function to handle window resize events
void handle_window_resize(SDL_Window* window, Plot* plot, TTF_Font* font, TTF_Font* title_font) {
    // Get the new window size
    SDL_GetWindowSize(window, &plot->window_width, &plot->window_height);
    
    // Recalculate layout
    calculate_layout(plot, font, title_font);
    
    printf("Window resized: %d x %d\n", plot->window_width, plot->window_height);
}

// MODIFIED: Function to handle mouse wheel events for zooming
void handle_mouse_wheel(Plot* plot, SDL_MouseWheelEvent wheel) {
    // Zoom in/out
    double zoom_speed = 0.1;
    if (wheel.y > 0) {
        // Zoom in
        plot->zoom_factor *= (1.0 + zoom_speed);
    } else if (wheel.y < 0) {
        // Zoom out
        plot->zoom_factor *= (1.0 - zoom_speed);
    }
    
    // Limit zoom factor
    if (plot->zoom_factor < 0.1) plot->zoom_factor = 0.1;
    if (plot->zoom_factor > 10.0) plot->zoom_factor = 10.0;
}

// MODIFIED: Function to handle mouse motion events for panning
void handle_mouse_motion(Plot* plot, SDL_MouseMotionEvent motion, bool* dragging) {
    // Only pan if mouse is inside plot area
    if (!point_in_rect(motion.x, motion.y, plot->plot_area)) {
        *dragging = false;
        return;
    }
    
    if (motion.state & SDL_BUTTON_LMASK) {
        // Left button is pressed - pan the view
        if (!*dragging) {
            *dragging = true;
        } else {
            // FIXED: Proper panning that doesn't affect scaling
            // Convert mouse movement to data space
            double dx = motion.xrel / (double)plot->plot_area.width;
            double dy = motion.yrel / (double)plot->plot_area.height;
            
            // Scale by the current view range
            double x_range = (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
            double y_range = (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
            
            // Update pan values (note: y is inverted in screen coordinates)
            plot->pan_x -= dx * x_range;
            plot->pan_y += dy * y_range;
        }
    } else {
        *dragging = false;
    }
}

// Function to handle key events
void handle_key_event(Plot* plot, SDL_KeyboardEvent key, SDL_Window* window) {
    if (key.type == SDL_KEYDOWN) {
        switch (key.keysym.sym) {
            case SDLK_r:
                // Reset view
                plot->zoom_factor = 1.0;
                plot->pan_x = 0.0;
                plot->pan_y = 0.0;
                break;
                
            case SDLK_g:
                // Toggle grid
                plot->show_grid = !plot->show_grid;
                break;
                
            case SDLK_l:
                // Toggle legend
                plot->show_legend = !plot->show_legend;
                break;
                
            case SDLK_a:
                // Toggle auto-scale
                plot->auto_scale = !plot->auto_scale;
                if (plot->auto_scale) {
                    plot_auto_scale(plot);
                }
                break;
                
            case SDLK_x:
                // Toggle x-axis scale (linear/log)
                plot->x_range.scale_type = (plot->x_range.scale_type == SCALE_LINEAR) ? SCALE_LOG : SCALE_LINEAR;
                if (plot->auto_scale) {
                    plot_auto_scale(plot);
                }
                break;
                
            case SDLK_y:
                // Toggle y-axis scale (linear/log)
                plot->y_range.scale_type = (plot->y_range.scale_type == SCALE_LINEAR) ? SCALE_LOG : SCALE_LINEAR;
                if (plot->auto_scale) {
                    plot_auto_scale(plot);
                }
                break;
            case SDLK_SPACE:
                // Toggle pause
                plot->IsPaused = !plot->IsPaused;
                break;
                
            case SDLK_F11:
            case SDLK_f:
                toggle_fullscreen(window, plot);
                break;
        }
    }
}

// MODIFIED: Function to draw a bar plot with clipping
void draw_bar_plot(SDL_Renderer* renderer, Plot* plot, DataSeries* series) {
    int plot_width = plot->plot_area.width;
    int bar_width = plot_width / (series->data_length * 2);
    
    for (int i = 0; i < series->data_length; i++) {
        double x_val = series->x_data[i];
        double y_val = series->y_data[i];
        
        // Apply zoom and pan
        double adjusted_x_min = plot->x_range.min + plot->pan_x;
        double adjusted_x_max = plot->x_range.max + plot->pan_x;
        double adjusted_y_min = plot->y_range.min + plot->pan_y;
        double adjusted_y_max = plot->y_range.max + plot->pan_y;
        
        // Scale the range based on zoom factor
        double x_range_center = (adjusted_x_min + adjusted_x_max) / 2;
        double y_range_center = (adjusted_y_min + adjusted_y_max) / 2;
        double x_range_half = (adjusted_x_max - adjusted_x_min) / (2 * plot->zoom_factor);
        double y_range_half = (adjusted_y_max - adjusted_y_min) / (2 * plot->zoom_factor);
        
        adjusted_x_min = x_range_center - x_range_half;
        adjusted_x_max = x_range_center + x_range_half;
        adjusted_y_min = y_range_center - y_range_half;
        adjusted_y_max = y_range_center + y_range_half;
        
        int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, 
                              plot->plot_area.x, plot->plot_area.x + plot->plot_area.width, 
                              plot->x_range.scale_type);
        int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, 
                              plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                              plot->y_range.scale_type);
        int y_zero = (int)map_value(0, adjusted_y_min, adjusted_y_max, 
                                   plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                                   plot->y_range.scale_type);
        
        // Ensure y_zero is within plot area
        if (y_zero < plot->plot_area.y) y_zero = plot->plot_area.y;
        if (y_zero > plot->plot_area.y + plot->plot_area.height) y_zero = plot->plot_area.y + plot->plot_area.height;
        
        // Skip if bar is completely outside plot area
        if (x + bar_width/2 < plot->plot_area.x || x - bar_width/2 > plot->plot_area.x + plot->plot_area.width) {
            continue;
        }
        
        // Clip bar to plot area
        int bar_left = x - bar_width/2;
        int bar_right = x + bar_width/2;
        int bar_top = (y < y_zero) ? y : y_zero;
        int bar_bottom = (y < y_zero) ? y_zero : y;
        
        if (bar_left < plot->plot_area.x) bar_left = plot->plot_area.x;
        if (bar_right > plot->plot_area.x + plot->plot_area.width) bar_right = plot->plot_area.x + plot->plot_area.width;
        if (bar_top < plot->plot_area.y) bar_top = plot->plot_area.y;
        if (bar_bottom > plot->plot_area.y + plot->plot_area.height) bar_bottom = plot->plot_area.y + plot->plot_area.height;
        
        // Draw bar
        boxRGBA(renderer, bar_left, bar_top, bar_right, bar_bottom,
               series->color.r, series->color.g, series->color.b, series->color.a);
        
        // Draw outline
        rectangleRGBA(renderer, bar_left, bar_top, bar_right, bar_bottom,
                     series->color.r * 0.8, series->color.g * 0.8, series->color.b * 0.8, series->color.a);
    }
}

// MODIFIED: Function to draw a stem plot with clipping
void draw_stem_plot(SDL_Renderer* renderer, Plot* plot, DataSeries* series) {
    for (int i = 0; i < series->data_length; i++) {
        double x_val = series->x_data[i];
        double y_val = series->y_data[i];
        
        // Apply zoom and pan
        double adjusted_x_min = plot->x_range.min + plot->pan_x;
        double adjusted_x_max = plot->x_range.max + plot->pan_x;
        double adjusted_y_min = plot->y_range.min + plot->pan_y;
        double adjusted_y_max = plot->y_range.max + plot->pan_y;
        
        // Scale the range based on zoom factor
        double x_range_center = (adjusted_x_min + adjusted_x_max) / 2;
        double y_range_center = (adjusted_y_min + adjusted_y_max) / 2;
        double x_range_half = (adjusted_x_max - adjusted_x_min) / (2 * plot->zoom_factor);
        double y_range_half = (adjusted_y_max - adjusted_y_min) / (2 * plot->zoom_factor);
        
        adjusted_x_min = x_range_center - x_range_half;
        adjusted_x_max = x_range_center + x_range_half;
        adjusted_y_min = y_range_center - y_range_half;
        adjusted_y_max = y_range_center + y_range_half;
        
        int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, 
                              plot->plot_area.x, plot->plot_area.x + plot->plot_area.width, 
                              plot->x_range.scale_type);
        int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, 
                              plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                              plot->y_range.scale_type);
        int y_zero = (int)map_value(0, adjusted_y_min, adjusted_y_max, 
                                   plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                                   plot->y_range.scale_type);
        
        // Ensure y_zero is within plot area
        if (y_zero < plot->plot_area.y) y_zero = plot->plot_area.y;
        if (y_zero > plot->plot_area.y + plot->plot_area.height) y_zero = plot->plot_area.y + plot->plot_area.height;
        
        // Skip if point is outside plot area in x direction
        if (x < plot->plot_area.x || x > plot->plot_area.x + plot->plot_area.width) {
            continue;
        }
        
        // Draw stem line with clipping
        int line_x1 = x;
        int line_y1 = y_zero;
        int line_x2 = x;
        int line_y2 = y;
        
        if (clip_line(&line_x1, &line_y1, &line_x2, &line_y2, plot->plot_area)) {
            lineRGBA(renderer, line_x1, line_y1, line_x2, line_y2,
                    series->color.r, series->color.g, series->color.b, series->color.a);
        }
        
        // Draw marker at the top if it's inside the plot area
        if (point_in_rect(x, y, plot->plot_area)) {
            draw_marker(renderer, x, y, 
                       series->marker_type != MARKER_NONE ? series->marker_type : MARKER_CIRCLE, 
                       series->marker_size, series->color, plot->plot_area);
        }
    }
}

// ADDED: Function to draw a heatmap
void draw_heatmap(SDL_Renderer* renderer, Plot* plot, DataSeries* series) {
    // Ensure the data series has valid data
    if (!series->x_data || !series->y_data || series->data_length <= 0) {
        return;
    }
    Matrix* heatmap_data = series->diffusion_data->M_voltage;
    // Calculate the number of rows and columns in the grid
    int rows = heatmap_data->rows;
    int cols = heatmap_data->cols;

    // Calculate the width and height of each cell
    int cell_width = (int)plot->plot_area.width / cols;
    int cell_height = (int)plot->plot_area.height / rows;

    if(cell_width < 1 || cell_height < 1) {
        cell_width = 1; // Avoid division by zero
        cell_height = 1; // Avoid division by zero
        printf("Warning: Cell size is too small, setting to minimum size of 1x1.\n");
    }

    // Iterate through the grid and draw each cell
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            // Calculate the data value for this cell
            double value = MAT(*heatmap_data, row, col);

            // Map the value to a color (e.g., using a gradient)
            Uint8 r = (Uint8)(255 * value);
            Uint8 g = (Uint8)(255 * (1 - value));
            Uint8 b = 128;
            Uint8 a = 255;

            // Calculate the position and size of the cell
            int x = plot->plot_area.x + col * cell_width;
            int y = plot->plot_area.y + row * cell_height;

            // Draw the cell
            SDL_SetRenderDrawColor(renderer, r, g, b, a);
            
            SDL_Rect cell_rect = {x, y, cell_width, cell_height};
            SDL_RenderFillRect(renderer, &cell_rect);
        }
    }
}

// Main plotting function
PlotError plot_show(Plot* plot) {
    if (plot == NULL || plot->series_count == 0) {
        return PLOT_ERROR_INVALID_DATA;
    }
    
    // Auto-scale if needed
    if (plot->auto_scale) {
        plot_auto_scale(plot);
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return PLOT_ERROR_SDL_INIT;
    }
    
    // Initialize SDL_ttf
    if (TTF_Init() < 0) {
        fprintf(stderr, "SDL_ttf could not initialize! TTF_Error: %s\n", TTF_GetError());
        SDL_Quit();
        return PLOT_ERROR_TTF_INIT;
    }
    
    // Create window with resizable flag
    SDL_Window* window = SDL_CreateWindow(plot->title, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         plot->window_width, 
                                         plot->window_height, 
                                         SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return PLOT_ERROR_WINDOW_CREATE;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);;
    if (renderer == NULL) {
        fprintf(stderr, "Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return PLOT_ERROR_RENDERER_CREATE;
    }
    
    // Load font
    TTF_Font* font = TTF_OpenFont(DEFAULT_FONT_PATH, DEFAULT_FONT_SIZE);
    TTF_Font* title_font = TTF_OpenFont(DEFAULT_FONT_PATH, DEFAULT_FONT_SIZE + 4);
    
    if (font == NULL || title_font == NULL) {
        fprintf(stderr, "Failed to load font! TTF_Error: %s\n", TTF_GetError());
        fprintf(stderr, "Trying to use a built-in font instead...\n");
        
        // If the default font fails, try a different path or use a fallback method
        font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", DEFAULT_FONT_SIZE);
        title_font = TTF_OpenFont("/usr/share/fonts/TTF/DejaVuSans.ttf", DEFAULT_FONT_SIZE + 4);
        
        if (font == NULL || title_font == NULL) {
            SDL_DestroyRenderer(renderer);
            SDL_DestroyWindow(window);
            TTF_Quit();
            SDL_Quit();
            return PLOT_ERROR_FONT_LOAD;
        }
    }
    
    // Calculate initial layout
    calculate_layout(plot, font, title_font);
    
    // Main loop flag
    bool quit = false;
    bool dragging = false;
    
    // Event handler
    SDL_Event e;
    
    // Main loop
    while (!quit) {
        // Handle events
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_MOUSEWHEEL) {
                handle_mouse_wheel(plot, e.wheel);
            } else if (e.type == SDL_MOUSEMOTION) {
                handle_mouse_motion(plot, e.motion, &dragging);
            } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
                handle_key_event(plot, e.key, window);
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_RESIZED || 
                    e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    handle_window_resize(window, plot, font, title_font);
                }
            }
        }
        
        // Clear screen with background color
        SDL_SetRenderDrawColor(renderer, 
                              plot->background_color.r, 
                              plot->background_color.g, 
                              plot->background_color.b, 
                              plot->background_color.a);
        SDL_RenderClear(renderer);
        
        // Apply zoom and pan
        double adjusted_x_min = plot->x_range.min + plot->pan_x;
        double adjusted_x_max = plot->x_range.max + plot->pan_x;
        double adjusted_y_min = plot->y_range.min + plot->pan_y;
        double adjusted_y_max = plot->y_range.max + plot->pan_y;
        
        // Scale the range based on zoom factor
        double x_range_center = (adjusted_x_min + adjusted_x_max) / 2;
        double y_range_center = (adjusted_y_min + adjusted_y_max) / 2;
        double x_range_half = (adjusted_x_max - adjusted_x_min) / (2 * plot->zoom_factor);
        double y_range_half = (adjusted_y_max - adjusted_y_min) / (2 * plot->zoom_factor);
        
        adjusted_x_min = x_range_center - x_range_half;
        adjusted_x_max = x_range_center + x_range_half;
        adjusted_y_min = y_range_center - y_range_half;
        adjusted_y_max = y_range_center + y_range_half;
        
        // Draw plot area border
        rectangleRGBA(renderer, 
                     plot->plot_area.x, plot->plot_area.y, 
                     plot->plot_area.x + plot->plot_area.width, plot->plot_area.y + plot->plot_area.height, 
                     plot->axis_color.r, plot->axis_color.g, plot->axis_color.b, plot->axis_color.a);
        
        // Draw grid lines if enabled
        if (plot->show_grid) {
            // Vertical grid lines
            for (int i = 1; i < GRID_LINES; i++) {
                int x = plot->plot_area.x + (i * plot->plot_area.width) / GRID_LINES;
                lineRGBA(renderer, x, plot->plot_area.y, x, plot->plot_area.y + plot->plot_area.height, 
                        plot->grid_color.r, plot->grid_color.g, plot->grid_color.b, plot->grid_color.a);
                
                // Draw x-axis labels
                char label[MAX_LABEL_LENGTH];
                double value;
                
                if (plot->x_range.scale_type == SCALE_LOG) {
                    // For logarithmic scale, use logarithmically spaced values
                    value = adjusted_x_min * pow(adjusted_x_max / adjusted_x_min, (double)i / GRID_LINES);
                } else {
                    // For linear scale, use linearly spaced values
                    value = adjusted_x_min + (adjusted_x_max - adjusted_x_min) * i / GRID_LINES;
                }
                
                snprintf(label, MAX_LABEL_LENGTH, "%.2g", value);
                render_text(renderer, font, label, x, plot->plot_area.y + plot->plot_area.height + 15, plot->text_color, true);
            }
            
            // Horizontal grid lines
            for (int i = 1; i < GRID_LINES; i++) {
                int y = plot->plot_area.y + (i * plot->plot_area.height) / GRID_LINES;
                lineRGBA(renderer, plot->plot_area.x, y, plot->plot_area.x + plot->plot_area.width, y, 
                        plot->grid_color.r, plot->grid_color.g, plot->grid_color.b, plot->grid_color.a);
                
                // Draw y-axis labels
                char label[MAX_LABEL_LENGTH];
                double value;
                
                if (plot->y_range.scale_type == SCALE_LOG) {
                    // For logarithmic scale, use logarithmically spaced values
                    value = adjusted_y_max * pow(adjusted_y_min / adjusted_y_max, (double)i / GRID_LINES);
                } else {
                    // For linear scale, use linearly spaced values
                    value = adjusted_y_max - (adjusted_y_max - adjusted_y_min) * i / GRID_LINES;
                }
                
                snprintf(label, MAX_LABEL_LENGTH, "%.2g", value);
                
                // FIXED: Position y-axis labels with proper spacing
                int label_width, label_height;
                get_text_dimensions(font, label, &label_width, &label_height);
                render_text(renderer, font, label, plot->plot_area.x - label_width - 5, y, plot->text_color, false);
            }
        }
        
        // Draw axis labels
        render_text(renderer, font, plot->x_label, 
                   plot->x_label_area.x + plot->x_label_area.width / 2, 
                   plot->x_label_area.y, 
                   plot->text_color, true);
        
        // Draw y-axis label (rotated text simulation)
        SDL_Surface* surface = TTF_RenderText_Blended(font, plot->y_label, 
                                                    (SDL_Color){plot->text_color.r, plot->text_color.g, plot->text_color.b, plot->text_color.a});
        if (surface) {
            int text_height = surface->h;
            SDL_FreeSurface(surface);
            
            // Rotate y-axis label
            render_rotated_text(renderer, font, plot->y_label, 
                       plot->y_label_area.x, 
                       plot->y_label_area.y + plot->y_label_area.height / 2, 
                       -90, plot->text_color);
        }
        
        // Draw title
        render_text(renderer, title_font, plot->title, 
                   plot->title_area.x + plot->title_area.width / 2, 
                   plot->title_area.y, 
                   plot->text_color, true);

        // Draw legend
        draw_legend(renderer, font, plot);
        
        // Draw help text
        char help_text[256];
        snprintf(help_text, sizeof(help_text), 
                "Mouse wheel: Zoom, Left drag: Pan, R: Reset view, G: Toggle grid, L: Legend, F: Fullscreen");
        render_text(renderer, font, help_text, plot->window_width / 2, plot->window_height - 15, plot->text_color, true);
        
        // Draw all data series
        for (int s = 0; s < plot->series_count; s++) {
            DataSeries* series = &plot->series[s];
            
            if (!series->visible || series->data_length <= 0) continue;
            
            // Update series data if dynamic
            if(series->dynamic_plot && !plot->IsPaused){

                if(series->diff_video_generator(series->ode_input, series->diffusion_data, series->frame_speed) != 0){
                    printf("Error generating data for series %d\n", s);
                    break;
                }
                series->y_data = series->diffusion_data->M_voltage->data; // Update the y_data. In principle x_data will be static.
                
                if(series->data_length != (series->diffusion_data->M_voltage->rows) * (series->diffusion_data->M_voltage->cols)){
                    printf("Error: data length mismatch for series %d\n", s);
                    break;
                }
                
            }

            // Set the drawing color
            SDL_SetRenderDrawColor(renderer, series->color.r, series->color.g, series->color.b, series->color.a);
            
            // Draw based on plot type
            switch (series->plot_type) {
                case PLOT_HEATMAP:
                    draw_heatmap(renderer, plot, series);
                    break;
                case PLOT_BAR:
                    draw_bar_plot(renderer, plot, series);
                    break;
                    
                case PLOT_STEM:
                    draw_stem_plot(renderer, plot, series);
                    break;
                    
                case PLOT_SCATTER:
                    // Draw only markers
                    for (int i = 0; i < series->data_length; i++) {
                        double x_val = series->x_data[i];
                        double y_val = series->y_data[i];
                        
                        int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, 
                                              plot->plot_area.x, plot->plot_area.x + plot->plot_area.width, 
                                              plot->x_range.scale_type);
                        int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, 
                                              plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                                              plot->y_range.scale_type);
                        
                        draw_marker(renderer, x, y, 
                                   series->marker_type != MARKER_NONE ? series->marker_type : MARKER_CIRCLE, 
                                   series->marker_size, series->color, plot->plot_area);
                    }
                    break;
                    
                case PLOT_LINE:
                default:
                    // Draw lines connecting data points
                    for (int i = 0; i < series->data_length - 1; i++) {
                        double x1_val = series->x_data[i];
                        double y1_val = series->y_data[i];
                        double x2_val = series->x_data[i+1];
                        double y2_val = series->y_data[i+1];
                        
                        int x1 = (int)map_value(x1_val, adjusted_x_min, adjusted_x_max, 
                                               plot->plot_area.x, plot->plot_area.x + plot->plot_area.width, 
                                               plot->x_range.scale_type);
                        int y1 = (int)map_value(y1_val, adjusted_y_min, adjusted_y_max, 
                                               plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                                               plot->y_range.scale_type);
                        int x2 = (int)map_value(x2_val, adjusted_x_min, adjusted_x_max, 
                                               plot->plot_area.x, plot->plot_area.x + plot->plot_area.width, 
                                               plot->x_range.scale_type);
                        int y2 = (int)map_value(y2_val, adjusted_y_min, adjusted_y_max, 
                                               plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                                               plot->y_range.scale_type);
                        
                        draw_line(renderer, x1, y1, x2, y2, 
                                 series->line_style, series->line_width, series->color, plot->plot_area);
                    }
                    
                    // Draw markers if specified
                    if (series->marker_type != MARKER_NONE) {
                        for (int i = 0; i < series->data_length; i++) {
                            double x_val = series->x_data[i];
                            double y_val = series->y_data[i];
                            
                            int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, 
                                                  plot->plot_area.x, plot->plot_area.x + plot->plot_area.width, 
                                                  plot->x_range.scale_type);
                            int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, 
                                                  plot->plot_area.y + plot->plot_area.height, plot->plot_area.y, 
                                                  plot->y_range.scale_type);
                            
                            draw_marker(renderer, x, y, series->marker_type, series->marker_size, series->color, plot->plot_area);
                        }
                    }
                    break;
            }
        }
        
        // Update screen
        SDL_RenderPresent(renderer);
        
        // Cap to 60 FPS
        SDL_Delay(16);
    }
    
    // Clean up
    TTF_CloseFont(font);
    TTF_CloseFont(title_font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    
    return PLOT_SUCCESS;
}

// Function to clean up a plot
void plot_cleanup(Plot* plot) {
    if (plot == NULL) return;
    
    // Free all data series
    for (int i = 0; i < plot->series_count; i++) {
        free(plot->series[i].x_data);
        free(plot->series[i].y_data);
    }
    
    // Reset plot
    plot->series_count = 0;
}

#endif