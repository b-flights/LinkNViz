#include <linknviz.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

void* build_graphs(void*) {
    printf("Visualization start.\n");
    viz_bar bar_1 = {
        20,
        100,
        {30, 300},
        false,
        true,
        false,
        {180, 50, 30}
    };
    create_viz_bar("Bar 1", bar_1);

    viz_bar bar_2 = {
        20,
        200,
        {60, 300},
        false,
        true,
        false,
        {80, 200, 50}
    };
    create_viz_bar("Bar 2", bar_2);

    viz_bar bar_3 = {
        20,
        250,
        {90, 300},
        false,
        true,
        false,
        {150, 50, 215}
    };
    create_viz_bar("Bar 3", bar_3);

    viz_label label_1 = {
        "Bar Chart",
        {30, 310},
        25.0,
        false,
        0,
        {255, 255, 255}
    };
    create_viz_label("Bar Chart Label", label_1);

    viz_label label_2 = {
        "Pictogram",
        {180, 310},
        25.0,
        false,
        0,
        {255, 255, 255}
    };
    create_viz_label("Pictogram Label", label_2);

    viz_label label_3 = {
        "Sector",
        {350, 310},
        25.0,
        false,
        0,
        {255, 255, 255}
    };
    create_viz_label("Sector Label", label_3);

    viz_sector sector_1 = {
        0.8,
        40,
        {390, 260},
        0,
        {80, 180, 255, 255}
    };
    create_viz_sector("Sector", sector_1);

    float x_pos[] = {5.0, 10.0, 15.0, 20.0, 24.0, 30.0, 25.0, 22.0, 18.0, 14.0, 7.0, 5.0, 0.0};
    float y_pos[] = {17.0, 17.0, 5.0, 20.0, 20.0, 5.0, 5.0, 15.0, -5.0, -5.0, 12.0, 2.0, 2.0};

    viz_point poly_points[200];

    for (int i = 0; i < 13; i++) {
        poly_points[i].x = x_pos[i];
        poly_points[i].y = y_pos[i];
    }
    int graphic_id = create_graphic();
    add_poly(poly_points, 13, graphic_id, (viz_color){255, 80, 50, 255});

    viz_pictogram pict = {
        7,
        0,
        {180, 300},
        3,
        VERTICAL,
        3
    };

    int pict_id = create_viz_pictogram("Pictogram", pict);
    char buf[128];
    while (viz_running()) {
        printf("How many pictogram elements: ");
        fgets(buf, 128, stdin);
        get_viz_pictogram(pict_id)->value = atoi(buf);
    }
}

int main(int argc, char *argv[]) {
    printf("Program start.\n");
    viz_init(argc, argv, 460, 360, (viz_color){20, 40, 60, 255}, "KumbhSans.ttf", build_graphs);
    return 0;
}
