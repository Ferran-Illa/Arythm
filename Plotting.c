#include "include/common.h"
#include "include/plotting.h"

#ifndef PLOTTING_C
#define PLOTTING_C
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
    
    return PLOT_SUCCESS;
}

// Function to add a data series to a plot
PlotError plot_add_series(Plot* plot, Vector* x_vec, Vector* y_vec, const char* label, 
                            Color color, LineStyle line_style, MarkerType marker_type,
                            int line_width, int marker_size, PlotType plot_type) 
{

    if (x_vec == NULL || y_vec == NULL) {
        return PLOT_ERROR_INVALID_VECTOR;
    }
    if(x_vec->size != y_vec->size) {
        return PLOT_ERROR_SIZE_MISMATCH;
    }

    const double *x_data = x_vec -> data;
    const double *y_data = y_vec -> data;

    int data_length = x_vec -> size;

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
void draw_marker(SDL_Renderer* renderer, int x, int y, MarkerType marker_type, int size, Color color) {
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

// Function to draw a line with the specified style
void draw_line(SDL_Renderer* renderer, int x1, int y1, int x2, int y2, LineStyle style, int width, Color color) {
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
                
                filledCircleRGBA(renderer, dot_x, dot_y, width/2 + 1, 
                                color.r, color.g, color.b, color.a);
                
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
                
                if (width > 1) {
                    thickLineRGBA(renderer, dash_x1, dash_y1, dash_x2, dash_y2, width, 
                                 color.r, color.g, color.b, color.a);
                } else {
                    lineRGBA(renderer, dash_x1, dash_y1, dash_x2, dash_y2, 
                            color.r, color.g, color.b, color.a);
                }
                
                pos = end_pos + gap_length;
                
                // Draw dot
                if (pos < length) {
                    int dot_x = x1 + (int)(pos * cos(angle));
                    int dot_y = y1 + (int)(pos * sin(angle));
                    
                    filledCircleRGBA(renderer, dot_x, dot_y, width/2 + 1, 
                                    color.r, color.g, color.b, color.a);
                    
                    pos += dot_length + gap_length;
                }
            }
            break;
        }
    }
}

// Function to render text with proper font
void render_text(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, Color color, bool center) {
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
}

// Function to draw the legend
void draw_legend(SDL_Renderer* renderer, TTF_Font* font, Plot* plot) {
    if (!plot->show_legend || plot->series_count == 0) return;
    
    int legend_x = WINDOW_WIDTH - MARGIN - 150;
    int legend_y = MARGIN + 20;
    int line_length = 20;
    int item_height = 25;
    
    // Draw legend background
    boxRGBA(renderer, legend_x - 10, legend_y - 10, 
           legend_x + 160, legend_y + plot->series_count * item_height + 10, 
           240, 240, 240, 200);
    
    rectangleRGBA(renderer, legend_x - 10, legend_y - 10, 
                 legend_x + 160, legend_y + plot->series_count * item_height + 10, 
                 100, 100, 100, 255);
    
    // Draw legend items
    for (int i = 0; i < plot->series_count; i++) {
        DataSeries* series = &plot->series[i];
        int y = legend_y + i * item_height;
        
        // Draw line/marker sample
        if (series->plot_type == PLOT_LINE || series->plot_type == PLOT_SCATTER || series->plot_type == PLOT_STEM) {
            if (series->plot_type != PLOT_SCATTER && series->line_style != MARKER_NONE) {
                draw_line(renderer, legend_x, y + item_height/2, legend_x + line_length, y + item_height/2, 
                         series->line_style, series->line_width, series->color);
            }
            
            if (series->marker_type != MARKER_NONE) {
                draw_marker(renderer, legend_x + line_length/2, y + item_height/2, 
                           series->marker_type, series->marker_size, series->color);
            }
        } else if (series->plot_type == PLOT_BAR) {
            boxRGBA(renderer, legend_x, y + item_height/4, legend_x + line_length, y + 3*item_height/4, 
                   series->color.r, series->color.g, series->color.b, series->color.a);
        }
        
        // Draw label
        render_text(renderer, font, series->label, legend_x + line_length + 10, y + item_height/2 - 8, 
                   plot->text_color, false);
    }
}

// Function to handle mouse wheel events for zooming
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

// Function to handle mouse motion events for panning
void handle_mouse_motion(Plot* plot, SDL_MouseMotionEvent motion, bool* dragging) {
    if (motion.state & SDL_BUTTON_LMASK) {
        // Left button is pressed - pan the view
        if (!*dragging) {
            *dragging = true;
        } else {
            double pan_speed = 0.01;
            double dx = motion.xrel * pan_speed;
            double dy = motion.yrel * pan_speed;
            
            plot->pan_x -= dx;
            plot->pan_y += dy;
        }
    } else {
        *dragging = false;
    }
}

// Function to handle key events
void handle_key_event(Plot* plot, SDL_KeyboardEvent key) {
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
        }
    }
}

// Function to draw a bar plot
void draw_bar_plot(SDL_Renderer* renderer, Plot* plot, DataSeries* series) {
    int bar_width = PLOT_WIDTH / (series->data_length * 2);
    
    for (int i = 0; i < series->data_length; i++) {
        double x_val = series->x_data[i];
        double y_val = series->y_data[i];
        
        // Apply zoom and pan
        double adjusted_x_min = plot->x_range.min - plot->pan_x * (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
        double adjusted_x_max = plot->x_range.max + plot->pan_x * (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
        double adjusted_y_min = plot->y_range.min - plot->pan_y * (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
        double adjusted_y_max = plot->y_range.max + plot->pan_y * (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
        
        // Scale the range based on zoom factor
        double x_range_center = (adjusted_x_min + adjusted_x_max) / 2;
        double y_range_center = (adjusted_y_min + adjusted_y_max) / 2;
        double x_range_half = (adjusted_x_max - adjusted_x_min) / (2 * plot->zoom_factor);
        double y_range_half = (adjusted_y_max - adjusted_y_min) / (2 * plot->zoom_factor);
        
        adjusted_x_min = x_range_center - x_range_half;
        adjusted_x_max = x_range_center + x_range_half;
        adjusted_y_min = y_range_center - y_range_half;
        adjusted_y_max = y_range_center + y_range_half;
        
        int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, MARGIN, WINDOW_WIDTH - MARGIN, plot->x_range.scale_type);
        int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
        int y_zero = (int)map_value(0, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
        
        // Ensure y_zero is within plot area
        if (y_zero < MARGIN) y_zero = MARGIN;
        if (y_zero > WINDOW_HEIGHT - MARGIN) y_zero = WINDOW_HEIGHT - MARGIN;
        
        // Draw bar
        if (y < y_zero) {
            boxRGBA(renderer, x - bar_width/2, y, x + bar_width/2, y_zero, 
                   series->color.r, series->color.g, series->color.b, series->color.a);
        } else {
            boxRGBA(renderer, x - bar_width/2, y_zero, x + bar_width/2, y, 
                   series->color.r, series->color.g, series->color.b, series->color.a);
        }
        
        // Draw outline
        rectangleRGBA(renderer, x - bar_width/2, y < y_zero ? y : y_zero, x + bar_width/2, y < y_zero ? y_zero : y, 
                     series->color.r * 0.8, series->color.g * 0.8, series->color.b * 0.8, series->color.a);
    }
}

// Function to draw a stem plot
void draw_stem_plot(SDL_Renderer* renderer, Plot* plot, DataSeries* series) {
    for (int i = 0; i < series->data_length; i++) {
        double x_val = series->x_data[i];
        double y_val = series->y_data[i];
        
        // Apply zoom and pan
        double adjusted_x_min = plot->x_range.min - plot->pan_x * (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
        double adjusted_x_max = plot->x_range.max + plot->pan_x * (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
        double adjusted_y_min = plot->y_range.min - plot->pan_y * (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
        double adjusted_y_max = plot->y_range.max + plot->pan_y * (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
        
        // Scale the range based on zoom factor
        double x_range_center = (adjusted_x_min + adjusted_x_max) / 2;
        double y_range_center = (adjusted_y_min + adjusted_y_max) / 2;
        double x_range_half = (adjusted_x_max - adjusted_x_min) / (2 * plot->zoom_factor);
        double y_range_half = (adjusted_y_max - adjusted_y_min) / (2 * plot->zoom_factor);
        
        adjusted_x_min = x_range_center - x_range_half;
        adjusted_x_max = x_range_center + x_range_half;
        adjusted_y_min = y_range_center - y_range_half;
        adjusted_y_max = y_range_center + y_range_half;
        
        int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, MARGIN, WINDOW_WIDTH - MARGIN, plot->x_range.scale_type);
        int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
        int y_zero = (int)map_value(0, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
        
        // Ensure y_zero is within plot area
        if (y_zero < MARGIN) y_zero = MARGIN;
        if (y_zero > WINDOW_HEIGHT - MARGIN) y_zero = WINDOW_HEIGHT - MARGIN;
        
        // Draw stem line
        lineRGBA(renderer, x, y_zero, x, y, 
                series->color.r, series->color.g, series->color.b, series->color.a);
        
        // Draw marker at the top
        draw_marker(renderer, x, y, series->marker_type != MARKER_NONE ? series->marker_type : MARKER_CIRCLE, 
                   series->marker_size, series->color);
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
    
    // Create window
    SDL_Window* window = SDL_CreateWindow(plot->title, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         SDL_WINDOWPOS_UNDEFINED, 
                                         WINDOW_WIDTH, 
                                         WINDOW_HEIGHT, 
                                         SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return PLOT_ERROR_WINDOW_CREATE;
    }
    
    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
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
                handle_key_event(plot, e.key);
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
        double adjusted_x_min = plot->x_range.min - plot->pan_x * (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
        double adjusted_x_max = plot->x_range.max + plot->pan_x * (plot->x_range.max - plot->x_range.min) / plot->zoom_factor;
        double adjusted_y_min = plot->y_range.min - plot->pan_y * (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
        double adjusted_y_max = plot->y_range.max + plot->pan_y * (plot->y_range.max - plot->y_range.min) / plot->zoom_factor;
        
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
                     MARGIN, MARGIN, 
                     WINDOW_WIDTH - MARGIN, WINDOW_HEIGHT - MARGIN, 
                     plot->axis_color.r, plot->axis_color.g, plot->axis_color.b, plot->axis_color.a);
        
        // Draw grid lines if enabled
        if (plot->show_grid) {
            // Vertical grid lines
            for (int i = 1; i < GRID_LINES; i++) {
                int x = MARGIN + (i * PLOT_WIDTH) / GRID_LINES;
                lineRGBA(renderer, x, MARGIN, x, WINDOW_HEIGHT - MARGIN, 
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
                render_text(renderer, font, label, x, WINDOW_HEIGHT - MARGIN + 15, plot->text_color, true);
            }
            
            // Horizontal grid lines
            for (int i = 1; i < GRID_LINES; i++) {
                int y = MARGIN + (i * PLOT_HEIGHT) / GRID_LINES;
                lineRGBA(renderer, MARGIN, y, WINDOW_WIDTH - MARGIN, y, 
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
                render_text(renderer, font, label, MARGIN - 30, y, plot->text_color, false);
            }
        }
        
        // Draw axis labels
        render_text(renderer, font, plot->x_label, WINDOW_WIDTH / 2, WINDOW_HEIGHT - MARGIN + 40, plot->text_color, true);
        
        // Rotate text for y-axis label (simulate rotation by positioning)
        SDL_Surface* surface = TTF_RenderText_Blended(font, plot->y_label, 
                                                    (SDL_Color){plot->text_color.r, plot->text_color.g, plot->text_color.b, plot->text_color.a});
        if (surface) {
            int text_height = surface->h;
            SDL_FreeSurface(surface);
            
            render_text(renderer, font, plot->y_label, MARGIN - 40, WINDOW_HEIGHT / 2 - text_height / 2, plot->text_color, false);
        }
        
        // Draw title
        render_text(renderer, title_font, plot->title, WINDOW_WIDTH / 2, MARGIN - 20, plot->text_color, true);
        
        // Draw all data series
        for (int s = 0; s < plot->series_count; s++) {
            DataSeries* series = &plot->series[s];
            
            if (!series->visible || series->data_length <= 0) continue;
            
            // Set the drawing color
            SDL_SetRenderDrawColor(renderer, series->color.r, series->color.g, series->color.b, series->color.a);
            
            // Draw based on plot type
            switch (series->plot_type) {
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
                        
                        int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, MARGIN, WINDOW_WIDTH - MARGIN, plot->x_range.scale_type);
                        int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
                        
                        draw_marker(renderer, x, y, series->marker_type != MARKER_NONE ? series->marker_type : MARKER_CIRCLE, 
                                   series->marker_size, series->color);
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
                        
                        int x1 = (int)map_value(x1_val, adjusted_x_min, adjusted_x_max, MARGIN, WINDOW_WIDTH - MARGIN, plot->x_range.scale_type);
                        int y1 = (int)map_value(y1_val, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
                        int x2 = (int)map_value(x2_val, adjusted_x_min, adjusted_x_max, MARGIN, WINDOW_WIDTH - MARGIN, plot->x_range.scale_type);
                        int y2 = (int)map_value(y2_val, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
                        
                        draw_line(renderer, x1, y1, x2, y2, series->line_style, series->line_width, series->color);
                    }
                    
                    // Draw markers if specified
                    if (series->marker_type != MARKER_NONE) {
                        for (int i = 0; i < series->data_length; i++) {
                            double x_val = series->x_data[i];
                            double y_val = series->y_data[i];
                            
                            int x = (int)map_value(x_val, adjusted_x_min, adjusted_x_max, MARGIN, WINDOW_WIDTH - MARGIN, plot->x_range.scale_type);
                            int y = (int)map_value(y_val, adjusted_y_min, adjusted_y_max, WINDOW_HEIGHT - MARGIN, MARGIN, plot->y_range.scale_type);
                            
                            draw_marker(renderer, x, y, series->marker_type, series->marker_size, series->color);
                        }
                    }
                    break;
            }
        }
        
        // Draw legend
        draw_legend(renderer, font, plot);
        
        // Draw help text
        char help_text[256];
        snprintf(help_text, sizeof(help_text), 
                "Mouse wheel: Zoom, Left drag: Pan, R: Reset view, G: Toggle grid, L: Toggle legend, A: Auto-scale, X/Y: Toggle log scale");
        render_text(renderer, font, help_text, WINDOW_WIDTH / 2, WINDOW_HEIGHT - 15, plot->text_color, true);
        
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

#endif // PLOTTING_C
/*
// Example usage
int main() {
    // Initialize plot
    Plot plot;
    plot_init(&plot);
    
    // Set plot properties
    strcpy(plot.title, "Enhanced MATLAB-like Plot");
    strcpy(plot.x_label, "X-Axis");
    strcpy(plot.y_label, "Y-Axis");
    
    // Create some example data
    double x1[100], y1[100];
    double x2[50], y2[50];
    double x3[20], y3[20];
    double x4[10], y4[10];
    
    // Generate data for multiple series
    for (int i = 0; i < 100; i++) {
        x1[i] = i * 0.1;
        y1[i] = sin(x1[i]) * exp(-x1[i] * 0.1);
    }
    
    for (int i = 0; i < 50; i++) {
        x2[i] = i * 0.2;
        y2[i] = cos(x2[i]);
    }
    
    for (int i = 0; i < 20; i++) {
        x3[i] = i * 0.5;
        y3[i] = 0.5 * x3[i] * x3[i];
    }
    
    for (int i = 0; i < 10; i++) {
        x4[i] = i;
        y4[i] = i * 0.8;
    }
    
    // Add data series with different styles
    plot_add_series(&plot, x1, y1, 100, "Damped Sine", 
                   (Color){0, 0, 255, 255}, // Blue
                   LINE_SOLID, MARKER_CIRCLE, 2, 4, PLOT_LINE);
    
    plot_add_series(&plot, x2, y2, 50, "Cosine", 
                   (Color){255, 0, 0, 255}, // Red
                   LINE_DASHED, MARKER_SQUARE, 2, 4, PLOT_LINE);
    
    plot_add_series(&plot, x3, y3, 20, "Quadratic", 
                   (Color){0, 180, 0, 255}, // Green
                   LINE_DOTTED, MARKER_TRIANGLE, 2, 5, PLOT_LINE);
    
    plot_add_series(&plot, x4, y4, 10, "Linear", 
                   (Color){128, 0, 128, 255}, // Purple
                   LINE_DASH_DOT, MARKER_DIAMOND, 2, 5, PLOT_BAR);
    
    // Show the plot
    PlotError error = plot_show(&plot);
    if (error != PLOT_SUCCESS) {
        fprintf(stderr, "Error showing plot: %d\n", error);
    }
    
    // Clean up
    plot_cleanup(&plot);
    
    return 0;
}
    */