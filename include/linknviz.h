#include <stdbool.h>

#define MAX_OBJECTS 2048
#define MAX_GRAPHICS 512
#define MAX_NAME_LEN 32

/* An RGBA color used by visualizer objects. */
typedef struct {
    int r;
    int g;
    int b;
    int a; 
} viz_color;

/* The type of a visualizer object. */
enum viz_obj_type {
    VIZ_BAR,
    VIZ_PICTOGRAM,
    VIZ_SECTOR,
    VIZ_LABEL,
    VIZ_INSTANCE
};

/* Returns whether the current visualizer is active.*/
bool viz_running();

/* Start a visualizer.
 * 
 * 
 * argc: the argument count of the calling main function.
 * 
 * argv: the argument list of the calling main function.
 * 
 * w: the width of the visualizer.
 * 
 * h: the height of the visualizer.
 * 
 * color: the background color of the visualizer.
 * 
 * font_path: the path of the font used for labels (relative or absolute). */
int viz_init(
    int argc,
    char *argv[],
    int w,
    int h,
    viz_color color,
    char* font_path,
    void* (*func)(void*)
);

/* Returns the id of the object with the provided name, or -1 if it doesn't exist. */
int get_obj_id(const char *name);

/* Returns the type of the object with the provided id, or -1 if it doesn't exist. */
enum viz_obj_type get_obj_type(int id);

/* The object's orientation (used by pictograph objects). */
enum viz_orientation {
    HORIZONTAL,
    VERTICAL
};

/* A 2D point with x and y coordinates in pixels.
 * (0, 0) represents the top left pixel. */
typedef struct {
    int x;
    int y;
} viz_point;

/* A rectangular bar object.
 * 
 * 
 * width: the width of the bar in pixels.
 * 
 * height: the height of the bar in pixels.
 * 
 * origin: the location of the bar.
 * 
 * expand_left: whether the bar's width is drawn to the left of its origin.
 * 
 * expand_right: whether the bar's width is drawn to the right of its origin.
 * 
 * centered: whether the bar is centered around the origin (overrides
 * expand_left and expand_right).
 * 
 * color: the color used for the bar. */
typedef struct {
    int width;
    int height;
    viz_point origin;
    bool expand_left;
    bool expand_up;
    bool centered;
    viz_color color;
} viz_bar;

int create_viz_bar(const char *name, viz_bar bar);
viz_bar* get_viz_bar(int id);

/* A stack of repeated instances of a given graphic.
 * 
 * 
 * value: the number of instances of the graphic.
 * 
 * graphic_id: the id of the graphic to be used.
 * 
 * origin: the location of the pictogram.
 * 
 * margin: the space between graphic instances.
 * 
 * orientation: the orientation of the pictogram.
 * 
 * length: the number of instances on a single row if vertical, or
 * a single column if horizontal. */
typedef struct {
    int value;
    int graphic_id;
    viz_point origin;
    int margin;
    enum viz_orientation orientation;
    int length;
} viz_pictogram;

int create_viz_pictogram(const char *name, viz_pictogram pictogram);
viz_pictogram* get_viz_pictogram(int id);

/* A slice of a circle.
 * 
 * 
 * fraction: the fraction of a circle the sector makes up.
 * 
 * radius: the radius of the sector in pixels.
 * 
 * origin: the location of the sector.
 * 
 * angle: the rotation angle of the sector.
 * 
 * color: the color of the sector. */
typedef struct {
    float fraction;
    int radius;
    viz_point origin;
    int angle;
    viz_color color;
} viz_sector;

int create_viz_sector(const char *name, viz_sector sector);
viz_sector* get_viz_sector(int id);

/* A text label.
 * 
 * 
 * str: the text to be displayed.
 * 
 * origin: the location of the text.
 * 
 * size: the size of the text font.
 * 
 * centered: whether the label is centered around the origin.
 * 
 * angle: the rotation angle of the label.
 * 
 * color: the color of the label. */
typedef struct {
    char *str;
    viz_point origin;
    float size;
    bool centered;
    int angle;
    viz_color color;
} viz_label;

int create_viz_label(const char *name, viz_label label);
viz_label* get_viz_label(int id);

/* A single instance of a given graphic.
 * 
 * 
 * graphic_id: the id of the graphic.
 * 
 * origin: the location of the pictogram.
 * 
 * centered: whether the graphic is centered around the origin. */
typedef struct {
    int graphic_id;
    viz_point origin;
    bool centered;
} viz_instance;

int create_viz_instance(const char *name, viz_instance inst);
viz_instance* get_viz_instance(int id);

/* Creates an empty graphic and returns its id. */
int create_graphic();

/* Adds a polygon to a graphic.
 * 
 * 
 * points: the list of viz_points that make up the polygon. The provided polygon
 * must not be self-overlapping.
 * 
 * points_count: the number of points in the list.
 * 
 * id: the id of the graphic.
 * 
 * color: the color used for the current polygon.
 * 
 * Returns -1 for failure, and 0 for success. */
int add_poly(viz_point* points, int points_count, int id, viz_color color);
