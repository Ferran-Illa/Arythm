#include "common.h"

#ifndef PLOTTING_H
#define PLOTTING_H

#define DEFAULT_WINDOW_WIDTH 800
#define DEFAULT_WINDOW_HEIGHT 600
#define DEFAULT_MARGIN 70
#define TICK_SIZE 5
#define GRID_LINES 10
#define MAX_LABEL_LENGTH 64
#define MAX_DATA_SERIES 10
#define DEFAULT_FONT_SIZE 14
#define DEFAULT_FONT_PATH "/usr/share/fonts/truetype/msttcorefonts/times.ttf"


// Line styles
typedef enum {
    LINE_SOLID,
    LINE_DASHED,
    LINE_DOTTED,
    LINE_DASH_DOT
} LineStyle;

// Marker types
typedef enum {
    MARKER_NONE,
    MARKER_CIRCLE,
    MARKER_SQUARE,
    MARKER_TRIANGLE,
    MARKER_DIAMOND,
    MARKER_CROSS,
    MARKER_PLUS
} MarkerType;

// Scale types
typedef enum {
    SCALE_LINEAR,
    SCALE_LOG
} ScaleType;

// Plot types
typedef enum {
    PLOT_LINE,
    PLOT_SCATTER,
    PLOT_BAR,
    PLOT_STEM,
    PLOT_HEATMAP
} PlotType;

// Color structure
typedef struct {
    Uint8 r, g, b, a;
} Color;

// Range structure
typedef struct {
    double min;
    double max;
    ScaleType scale_type;
} Range;

// Data series structure
typedef struct {
    double* x_data;
    double* y_data;
    int data_length;

    char label[MAX_LABEL_LENGTH];
    Color color;
    LineStyle line_style;
    MarkerType marker_type;
    int line_width;
    int marker_size;
    PlotType plot_type;

    DiffVideo diff_video_generator; // Function pointer for diffusion video generator
    DiffusionData* diffusion_data; // Data for diffusion video
    OdeFunctionParams* ode_input; // Diffusion video ODE setup
    int frame_speed; // Speed of the video (computed iterations per frame)

    bool dynamic_plot; // 1D or 2D
    bool visible;
} DataSeries;

// Rectangle structure for layout
typedef struct {
    int x;
    int y;
    int width;
    int height;
} Rect;

// Plot structure
typedef struct {
    char title[MAX_LABEL_LENGTH];
    char x_label[MAX_LABEL_LENGTH];
    char y_label[MAX_LABEL_LENGTH];
    Range x_range;
    Range y_range;
    DataSeries series[MAX_DATA_SERIES];
    int series_count;
    bool show_grid;
    bool show_legend;
    Color background_color;
    Color grid_color;
    Color text_color;
    Color axis_color;
    double zoom_factor;
    double pan_x;
    double pan_y;
    bool auto_scale;
    bool IsPaused; // Flag to pause the plot if it has a video
    
    // Fullscreen related properties
    bool fullscreen;         // Flag to track fullscreen state
    int window_width;        // Current window width
    int window_height;       // Current window height
    int margin_left;         // Left margin (may be larger for y-axis labels)
    int margin_right;        // Right margin
    int margin_top;          // Top margin
    int margin_bottom;       // Bottom margin (may be larger for x-axis labels)

    // ADDED: Layout rectangles for different areas
    Rect plot_area;          // The actual plotting area
    Rect legend_area;        // Area for the legend
    Rect title_area;         // Area for the title
    Rect x_label_area;       // Area for x-axis label
    Rect y_label_area;       // Area for y-axis label
} Plot;

// Error handling
typedef enum {
    PLOT_SUCCESS,
    PLOT_ERROR_SDL_INIT,
    PLOT_ERROR_TTF_INIT,
    PLOT_ERROR_WINDOW_CREATE,
    PLOT_ERROR_RENDERER_CREATE,
    PLOT_ERROR_FONT_LOAD,
    PLOT_ERROR_INVALID_DATA,
    PLOT_ERROR_MAX_SERIES,
    PLOT_ERROR_SIZE_MISMATCH,
    PLOT_ERROR_INVALID_VECTOR,
} PlotError;

#ifndef PLOTTING_C
    extern PlotError plot_init(Plot* plot);
    extern PlotError plot_show(Plot* plot);
    PlotError plot_add_series(Plot* plot, Vector* x_vec, Vector* y_vec, const char* label, 
                            Color color, LineStyle line_style, MarkerType marker_type,
                            int line_width, int marker_size, PlotType plot_type);
    PlotError plot_config_video(Plot* plot, bool dynamic_plot, 
                            DiffVideo diff_video_generator, DiffusionData* diffusion_data, 
                            OdeFunctionParams* ode_input, int frame_speed);
    extern void plot_cleanup(Plot* plot);
#endif // PLOTTING_C

#endif // PLOTTING_H