#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_iostream.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <linknviz.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_NAME_LEN 32

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

int window_width;
int window_height;

char *label_font_path;

SDL_FColor get_sdl_fcolor(viz_color color) {
    SDL_FColor sdl_color;

    sdl_color.r = (float)color.r / 255;
    sdl_color.g = (float)color.g / 255;
    sdl_color.b = (float)color.b / 255;
    sdl_color.a = (float)color.a / 255;

    return sdl_color;
}

SDL_Color get_sdl_color(viz_color color) {
    SDL_Color sdl_color;

    sdl_color.r = color.r;
    sdl_color.g = color.g;
    sdl_color.b = color.b;
    sdl_color.a = color.a;

    return sdl_color;
}

typedef struct {
    char name[MAX_NAME_LEN + 1];
    enum viz_obj_type type;
    union {
        viz_pictogram pictogram;
        viz_bar bar;
        viz_sector sector;
        viz_label label;
        viz_instance inst;
    } viz_obj;
} viz_container;

typedef struct {
    char name[MAX_NAME_LEN + 1];
    int num_tris;
    SDL_Vertex *verts;
    viz_point min;
    viz_point max;
} viz_graphic;

viz_container viz_objs[2048];
int viz_obj_count = 0;

viz_graphic viz_graphics[2048];
int viz_graphic_count = 0;

viz_color window_color;

bool running = false;

bool viz_running() {
    return running;
}

int get_obj_id(const char *name) {
    for (int i = 0; i < viz_obj_count; i++) {
        if (strncmp(viz_objs[i].name, name, MAX_NAME_LEN) == 0) {
            return i;
        }
    }
    return -1;
}

enum viz_obj_type get_obj_type(int id) {
    if (id >= viz_obj_count) {
        return -1;
    }
    return viz_objs[id].type;
}

struct point_node {
    float x;
    float y;
    struct point_node *prev;
    struct point_node *next;
};

int inside_tri(
    const struct point_node *v,
    const struct point_node *v1,
    const struct point_node *v2,
    const struct point_node *v3
) {
    int side1 = ((v->x - v2->x) * (v1->y - v2->y) - (v1->x - v2->x) * (v->y - v2->y)) >= 0;
    int side2 = ((v->x - v3->x) * (v2->y - v3->y) - (v2->x - v3->x) * (v->y - v3->y)) >= 0;
    int side3 = ((v->x - v1->x) * (v3->y - v1->y) - (v3->x - v1->x) * (v->y - v1->y)) >= 0;

    if ((side1 == side2) && (side2 == side3)) {
        return 1;
    }
    return 0;
}

/* Check if vertices are in clockwise order. Returns 1 if clockwise, -1 if counter clockwise, and 0
 * if collinear. */
int is_clockwise(
    const struct point_node *v1,
    const struct point_node *v2,
    const struct point_node *v3
) {
    int stat = ((v2->y - v1->y) * (v3->x - v2->x) - (v2->x - v1->x) * (v3->y - v2->y));

    if (stat > 0) {
        return 1;
    } else if (stat < 0) {
        return -1;
    }
    return 0;
}

/* Triangulates the polygon using the Ear Clipping algorithm and adds the triangles to the graphic
 * with the given id and color.*/
int add_poly(viz_point* points, int points_count, int id, viz_color color) {

    // Less than 3 points: invalid polygon
    if (points_count <= 3) {return -1;}

    // Graphic out of range
    if (0 > id >= viz_graphic_count) {return -1;}

    viz_graphic *graphic = &viz_graphics[id];

    // Doubly linked list structure representing the polygon
    struct point_node *nodes = malloc(sizeof(struct point_node) * points_count);
    if (nodes == NULL) {
        return -1;
    }

    // Allocate space for vertices of the new polygon
    if (graphic->num_tris == 0) {
        graphic->verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * (points_count - 2) * 3);
        graphic->min = points[0];
        graphic->max = points[0];
    } else {
        graphic->verts = (SDL_Vertex*)realloc(graphic->verts, sizeof(SDL_Vertex) * ((points_count - 2) * 3 + graphic->num_tris * 3));
    }
    SDL_Vertex *verts = &(graphic->verts[graphic->num_tris * 3]);

    int num_nodes = points_count;

    // Populate linked list structure with vertex coordinates
    for (int i = 0; i < num_nodes; i++) {
        nodes[i].x = points[i].x;
        nodes[i].y = points[i].y;

        // Update minimum and maximum coordinates of the graphic
        if (points[i].x > graphic->max.x) {graphic->max.x = points[i].x;}
        if (points[i].y > graphic->max.y) {graphic->max.y = points[i].y;}
        if (points[i].x < graphic->min.x) {graphic->min.x = points[i].x;}
        if (points[i].y < graphic->min.y) {graphic->min.y = points[i].y;}
    }

    // Set pointers to neighbouring nodes
    for (int i = 1; i < num_nodes - 1; i++) {
        nodes[i].prev = &nodes[i - 1];
        nodes[i].next = &nodes[i + 1];
    }

    nodes[0].prev = &nodes[num_nodes - 1];
    nodes[0].next = &nodes[1];

    nodes[num_nodes - 1].prev = &nodes[num_nodes - 2];
    nodes[num_nodes - 1].next = &nodes[0];

    struct point_node min_point = nodes[0];
    for (int i = 0; i < points_count; i++) {
        if (nodes[i].x < min_point.x) {
            min_point = nodes[i];
        } else if (nodes[i].x == min_point.x && nodes[i].y < min_point.y) {
            min_point = nodes[i];
        }
    }

    int base_dir = is_clockwise(min_point.prev, &min_point, min_point.next);

    int tri_ind = 0;
    struct point_node *cur_ear;
    cur_ear = &nodes[0];
    const struct point_node *cur;

    /* Remove ears (triangles formed by convex neighbouring pair of edges with no points inside)
     * from the polygon until there remains only a single triangle, adding each triangle to the
     * graphic. */
    while (num_nodes > 3)  {
        int is_ear = 1;
        if (is_clockwise(cur_ear->prev, cur_ear, cur_ear->next) != base_dir) {
            is_ear = 0;
        };
        if (is_ear) {
            cur = cur_ear->next->next;

            while (cur->next != cur_ear && is_ear) {
                int in = inside_tri(cur, cur_ear->prev, cur_ear, cur_ear->next);
                if (in) {
                    is_ear = 0;
                }
                cur = cur->next;
            }
            if (is_ear) {
                verts[tri_ind * 3].position.x = cur_ear->prev->x;
                verts[tri_ind * 3].position.y = cur_ear->prev->y;

                verts[tri_ind * 3 + 1].position.x = cur_ear->x;
                verts[tri_ind * 3 + 1].position.y = cur_ear->y;

                verts[tri_ind * 3 + 2].position.x = cur_ear->next->x;
                verts[tri_ind * 3 + 2].position.y = cur_ear->next->y;

                tri_ind += 1;
                cur_ear->prev->next = cur_ear->next;
                cur_ear->next->prev = cur_ear->prev;
                num_nodes -= 1;
            }
        }
        cur_ear = cur_ear->next;
    }

    verts[tri_ind * 3].position.x = cur_ear->prev->x;
    verts[tri_ind * 3].position.y = cur_ear->prev->y;

    verts[tri_ind * 3 + 1].position.x = cur_ear->x;
    verts[tri_ind * 3 + 1].position.y = cur_ear->y;

    verts[tri_ind * 3 + 2].position.x = cur_ear->next->x;
    verts[tri_ind * 3 + 2].position.y = cur_ear->next->y;

    free(nodes);

    for (int i = 0; i < (points_count - 2) * 3; i++) {
        verts[i].color = get_sdl_fcolor(color);
    }
    
    graphic->num_tris += (points_count - 2);
    return 0;
}

int create_viz_bar(const char *name, viz_bar bar) {
    if (strlen(name) > MAX_NAME_LEN || viz_obj_count > MAX_OBJECTS) {
        return -1;
    }
    strncpy(viz_objs[viz_obj_count].name, name, strlen(name) + 1);
    viz_objs[viz_obj_count].viz_obj.bar = bar;
    viz_objs[viz_obj_count].type = VIZ_BAR;
    
    viz_obj_count += 1;

    return viz_obj_count - 1;
}

int create_viz_pictogram(const char *name, viz_pictogram pictogram) {
    if (strlen(name) > MAX_NAME_LEN || viz_obj_count > MAX_OBJECTS) {
        return -1;
    }
    strncpy(viz_objs[viz_obj_count].name, name, strlen(name) + 1);
    viz_objs[viz_obj_count].viz_obj.pictogram = pictogram;
    viz_objs[viz_obj_count].type = VIZ_PICTOGRAM;

    viz_obj_count += 1;

    return viz_obj_count - 1;
}

int create_viz_sector(const char *name, viz_sector sector) {
    if (strlen(name) > MAX_NAME_LEN || viz_obj_count > MAX_OBJECTS) {
        return -1;
    }
    strncpy(viz_objs[viz_obj_count].name, name, strlen(name) + 1);
    viz_objs[viz_obj_count].viz_obj.sector = sector;
    viz_objs[viz_obj_count].type = VIZ_SECTOR;

    viz_obj_count += 1;

    return viz_obj_count - 1;
}

int create_viz_label(const char *name, viz_label label) {
    if (strlen(name) > MAX_NAME_LEN || viz_obj_count > MAX_OBJECTS) {
        return -1;
    }
    strncpy(viz_objs[viz_obj_count].name, name, strlen(name) + 1);
    viz_objs[viz_obj_count].viz_obj.label = label;
    viz_objs[viz_obj_count].type = VIZ_LABEL;

    viz_obj_count += 1;

    return viz_obj_count - 1;
}

int create_viz_instance(const char *name, viz_instance inst) {
    if (strlen(name) > MAX_NAME_LEN || viz_obj_count == MAX_OBJECTS) {
        return -1;
    }
    strncpy(viz_objs[viz_obj_count].name, name, strlen(name) + 1);
    viz_objs[viz_obj_count].viz_obj.inst = inst;
    viz_objs[viz_obj_count].type = VIZ_INSTANCE;

    viz_obj_count += 1;

    return viz_obj_count - 1;
}

int create_graphic() {
    if (viz_graphic_count == MAX_GRAPHICS) {
        return -1;
    }
    viz_graphics[0].num_tris = 0;
    viz_graphic_count += 1;

    return viz_graphic_count - 1;
}

viz_bar* get_viz_bar(int id) {
    if (id >= viz_obj_count) {
        return NULL;
    }
    if (viz_objs[id].type == VIZ_BAR) {
        return &viz_objs[id].viz_obj.bar;
    }
    return NULL;
}

viz_pictogram* get_viz_pictogram(int id) {
    if (id >= viz_obj_count) {
        return NULL;
    }
    if (viz_objs[id].type == VIZ_PICTOGRAM) {
        return &viz_objs[id].viz_obj.pictogram;
    }
    return NULL;
}

viz_sector* get_viz_sector(int id) {
    if (id >= viz_obj_count) {
        return NULL;
    }
    if (viz_objs[id].type == VIZ_SECTOR) {
        return &viz_objs[id].viz_obj.sector;
    }
    return NULL;
}

viz_label* get_viz_label(int id) {
    if (id >= viz_obj_count) {
        return NULL;
    }
    if (viz_objs[id].type == VIZ_LABEL) {
        return &viz_objs[id].viz_obj.label;
    }
    return NULL;
}

viz_instance* get_viz_instance(int id) {
    if (id >= viz_obj_count) {
        return NULL;
    }
    if (viz_objs[id].type == VIZ_INSTANCE) {
        return &viz_objs[id].viz_obj.inst;
    }
    return NULL;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_CreateWindowAndRenderer("Link N' Viz", window_width, window_height, 0, &window, &renderer)) {
        SDL_Log("%s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!TTF_Init()) {
        SDL_Log("%s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    return SDL_APP_CONTINUE;
}

void* (*main_fn)(void*);


SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    SDL_SetRenderDrawColor(renderer, window_color.r, window_color.g, window_color.b, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderScale(renderer, 1, 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (int i = 0; i < viz_obj_count; i++) {
        viz_graphic graphic;
        SDL_Vertex *mod_verts;
        switch (viz_objs[i].type) {
            case VIZ_BAR:
                viz_bar bar = viz_objs[i].viz_obj.bar;

                SDL_FRect rect = {
                    bar.origin.x,
                    bar.origin.y,
                    bar.width,
                    bar.height
                };
                if (bar.centered) {
                    rect.x -= rect.w / 2;
                    rect.y -= rect.h / 2;
                } else {
                    if (bar.expand_left) {
                        rect.x -= rect.w;
                    }
                    if (bar.expand_up) {
                        rect.y -= rect.h;
                    }
                }
                SDL_SetRenderDrawColor(
                    renderer,
                    bar.color.r,
                    bar.color.g,
                    bar.color.b,
                    SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(renderer, &rect);

                break;

            case VIZ_PICTOGRAM:
                viz_pictogram pictogram = viz_objs[i].viz_obj.pictogram;
                graphic = viz_graphics[pictogram.graphic_id];
                int w = graphic.max.x - graphic.min.x;
                int h = graphic.max.y - graphic.min.y;

                // Draw each graphic in pictogram
                for (int j = 0; j < pictogram.value; j++) {
                    mod_verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * graphic.num_tris * 3);
                    if (mod_verts == NULL) {
                        break;
                    }
                    memcpy(mod_verts, graphic.verts, sizeof(SDL_Vertex) * graphic.num_tris * 3);

                    // Transform coordinates in the graphic to the correct location
                    for (int k = 0; k < graphic.num_tris * 3; k++) {
                        mod_verts[k].position.x = graphic.verts[k].position.x 
                        + pictogram.origin.x 
                        - graphic.min.x;

                        mod_verts[k].position.y = graphic.verts[k].position.y 
                        + pictogram.origin.y 
                        - graphic.min.y;

                        if (pictogram.orientation == VERTICAL) {
                            mod_verts[k].position.x += (w + pictogram.margin) * (j % pictogram.length);
                            mod_verts[k].position.y -= (h + pictogram.margin) * (floor((float)j / pictogram.length) + 1);

                        } else {
                            mod_verts[k].position.x += (w + pictogram.margin) * (floor((float)j / pictogram.length));
                            mod_verts[k].position.y -= (h + pictogram.margin) * (j % pictogram.length + 1);

                        }
                    }
                    SDL_RenderGeometry(renderer, NULL, mod_verts, graphic.num_tris * 3, NULL, 0);
                    free(mod_verts);
                }

                break;

            case VIZ_SECTOR:
                viz_sector sector = viz_objs[i].viz_obj.sector;

                // Draw sector as triangular slices, each with a 1 degree interior angle
                for (int j = 0; j < sector.fraction * 360; j++) {
                    SDL_Vertex tri[3];

                    tri[0].position.x = sector.origin.x;
                    tri[0].position.y = sector.origin.y;
                    tri[0].color = get_sdl_fcolor(sector.color);

                    tri[1].position.x = sector.origin.x + cos((j + sector.angle) * (2 * M_PI / 360)) * sector.radius;
                    tri[1].position.y = sector.origin.y + sin((j + sector.angle) * (2 * M_PI / 360)) * sector.radius;
                    tri[1].color = get_sdl_fcolor(sector.color);

                    tri[2].position.x = sector.origin.x + cos((j + sector.angle + 1) * (2 * M_PI / 360)) * sector.radius;
                    tri[2].position.y = sector.origin.y + sin((j + sector.angle + 1) * (2 * M_PI / 360)) * sector.radius;
                    tri[2].color = get_sdl_fcolor(sector.color);

                    SDL_RenderGeometry(renderer, NULL, tri, 3, NULL, 0);
                }

                break;

            case VIZ_LABEL:
                viz_label label = viz_objs[i].viz_obj.label;

                SDL_Texture *label_texture = NULL;
                SDL_Surface *label_text;
                SDL_Color label_color;
                TTF_Font *label_font = NULL;

                label_font = TTF_OpenFont(label_font_path, label.size);

                if (!label_font) {
                    SDL_Log("%s\n", SDL_GetError());
                    return SDL_APP_FAILURE;
                }

                label_color = get_sdl_color(label.color);

                label_text = TTF_RenderText_Blended(label_font, label.str, 0, label_color);
                if (label_text) {
                    label_texture = SDL_CreateTextureFromSurface(renderer, label_text);
                    SDL_DestroySurface(label_text);
                }
                if (!label_texture) {
                    SDL_Log("%s\n", SDL_GetError());
                    return SDL_APP_FAILURE;
                }
                SDL_FRect label_dst;
                SDL_GetTextureSize(label_texture, &label_dst.w, &label_dst.h);

                label_dst.x = label.origin.x;
                label_dst.y = label.origin.y;
                SDL_FPoint label_ctr = {0, 0};

                if (label.centered) {
                    label_ctr.x = label_dst.w / 2;
                    label_ctr.y = label_dst.h / 2;
                    label_dst.x -= label_dst.w / 2;
                    label_dst.y -= label_dst.h / 2;
                }
                SDL_RenderTextureRotated(
                    renderer,
                    label_texture,
                    NULL,
                    &label_dst,
                    label.angle,
                    &label_ctr,
                    SDL_FLIP_NONE
                );
                SDL_DestroyTexture(label_texture);
                TTF_CloseFont(label_font);

                break;

            case VIZ_INSTANCE:
                viz_instance inst = viz_objs[i].viz_obj.inst;
                graphic = viz_graphics[inst.graphic_id];

                mod_verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * graphic.num_tris * 3);
                if (mod_verts == NULL) {
                    break;
                }
                memcpy(mod_verts, graphic.verts, sizeof(SDL_Vertex) * graphic.num_tris * 3);
                for (int j = 0; j < graphic.num_tris * 3; j++) {
                    mod_verts[j].position.x = graphic.verts[j].position.x + inst.origin.x;
                    mod_verts[j].position.y = graphic.verts[j].position.y + inst.origin.y;
                    if (inst.centered) {
                        mod_verts[j].position.x -= (graphic.max.x - graphic.min.x) / 2 + graphic.min.x;
                        mod_verts[j].position.y -= (graphic.max.y - graphic.min.y) / 2 + graphic.min.y;
                    }
                }
                SDL_RenderGeometry(renderer, NULL, mod_verts, graphic.num_tris * 3, NULL, 0);
                free(mod_verts);

                break;
        }
    }
    SDL_RenderPresent(renderer);
    
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    for (int i = 0; i < viz_graphic_count; i++) {
        free(viz_graphics[i].verts);
    }
    running = false;
}

pthread_t thread;

int viz_init(
    int argc,
    char *argv[],
    int w,
    int h,
    viz_color color,
    char* font_path,
    void* (*func)(void*)
) {
    window_width = w;
    window_height = h;
    window_color = color;
    label_font_path = font_path;
    main_fn = func;
    running = true;
    pthread_create(&thread, NULL, main_fn, NULL);
    SDL_EnterAppMainCallbacks(argc, argv, SDL_AppInit, SDL_AppIterate, SDL_AppEvent, SDL_AppQuit);
    pthread_join(thread, NULL);
    return 0;
}
