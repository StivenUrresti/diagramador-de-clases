#include "uml.h"

#include <stdio.h>
#include <string.h>

#ifdef USE_RAYLIB
#include <math.h>
#include <raylib.h>

static int class_index(const Diagram *diagram, const char *name) {
    for (size_t i = 0; i < diagram->class_count; i++) {
        if (strcmp(diagram->classes[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static Vector2 center_box(const ClassBox *box) {
    return (Vector2){(float)(box->x + box->width / 2), (float)(box->y + box->height / 2)};
}

static void get_relation_points(const Diagram *diagram, const Layout *layout, const RelationDecl *relation, Vector2 *points, size_t *count) {
    if (relation->point_count > 0) {
        *count = relation->point_count;
        for (size_t i = 0; i < relation->point_count; i++) {
            points[i] = (Vector2){(float)relation->points[i].x, (float)relation->points[i].y};
        }
        return;
    }

    int from = class_index(diagram, relation->from);
    int to = class_index(diagram, relation->to);
    *count = 0;
    if (from >= 0 && to >= 0) {
        points[(*count)++] = center_box(&layout->boxes[from]);
        points[(*count)++] = center_box(&layout->boxes[to]);
    }
}

static void draw_marker(RelationType type, Vector2 from, Vector2 to) {
    Vector2 dir = {to.x - from.x, to.y - from.y};
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (length < 1.0f) {
        return;
    }

    Vector2 u = {dir.x / length, dir.y / length};
    Vector2 p = {-u.y, u.x};

    if (type == REL_HERENCIA) {
        Vector2 a = {to.x - u.x * 18 + p.x * 9, to.y - u.y * 18 + p.y * 9};
        Vector2 b = {to.x - u.x * 18 - p.x * 9, to.y - u.y * 18 - p.y * 9};
        DrawTriangleLines(to, a, b, BLACK);
    } else if (type == REL_AGREGACION || type == REL_COMPOSICION) {
        Vector2 a = {to.x - u.x * 13 + p.x * 8, to.y - u.y * 13 + p.y * 8};
        Vector2 b = {to.x - u.x * 26, to.y - u.y * 26};
        Vector2 c = {to.x - u.x * 13 - p.x * 8, to.y - u.y * 13 - p.y * 8};
        if (type == REL_COMPOSICION) {
            DrawTriangle(to, a, b, BLACK);
            DrawTriangle(to, b, c, BLACK);
        }
        DrawLineV(to, a, BLACK);
        DrawLineV(a, b, BLACK);
        DrawLineV(b, c, BLACK);
        DrawLineV(c, to, BLACK);
    } else if (type == REL_DEPENDENCIA) {
        Vector2 a = {to.x - u.x * 14 + p.x * 7, to.y - u.y * 14 + p.y * 7};
        Vector2 b = {to.x - u.x * 14 - p.x * 7, to.y - u.y * 14 - p.y * 7};
        DrawLineV(a, to, BLACK);
        DrawLineV(to, b, BLACK);
    }
}

static void draw_dashed(Vector2 a, Vector2 b) {
    Vector2 dir = {b.x - a.x, b.y - a.y};
    float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
    if (length < 1.0f) {
        return;
    }
    Vector2 u = {dir.x / length, dir.y / length};
    for (float d = 0; d < length; d += 14.0f) {
        float end = d + 8.0f;
        if (end > length) {
            end = length;
        }
        DrawLineV((Vector2){a.x + u.x * d, a.y + u.y * d}, (Vector2){a.x + u.x * end, a.y + u.y * end}, BLACK);
    }
}

static void draw_relation_lines(const Diagram *diagram, const Layout *layout) {
    for (size_t r = 0; r < diagram->relation_count; r++) {
        Vector2 points[UML_MAX_POINTS];
        size_t count = 0;
        get_relation_points(diagram, layout, &diagram->relations[r], points, &count);
        for (size_t i = 1; i < count; i++) {
            if (diagram->relations[r].type == REL_DEPENDENCIA) {
                draw_dashed(points[i - 1], points[i]);
            } else {
                DrawLineV(points[i - 1], points[i], BLACK);
            }
        }
    }
}

static void draw_relation_markers(const Diagram *diagram, const Layout *layout) {
    for (size_t r = 0; r < diagram->relation_count; r++) {
        Vector2 points[UML_MAX_POINTS];
        size_t count = 0;
        get_relation_points(diagram, layout, &diagram->relations[r], points, &count);
        if (count >= 2) {
            draw_marker(diagram->relations[r].type, points[count - 2], points[count - 1]);
        }
    }
}

static void draw_diagram_raylib(const Diagram *diagram, const Layout *layout) {
    ClearBackground((Color){247, 247, 247, 255});
    DrawText(diagram->name, 20, 18, 18, BLACK);

    draw_relation_lines(diagram, layout);

    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        const ClassBox *box = &layout->boxes[i];
        int y_name = box->y + box->name_height;
        int y_attrs = y_name + box->attrs_height;

        DrawRectangle(box->x, box->y, box->width, box->height, WHITE);
        DrawRectangleLines(box->x, box->y, box->width, box->height, BLACK);
        DrawLine(box->x, y_name, box->x + box->width, y_name, BLACK);
        DrawLine(box->x, y_attrs, box->x + box->width, y_attrs, BLACK);
        DrawText(klass->name, box->x + 10, box->y + 9, 16, BLACK);
        if (klass->is_abstract) {
            DrawLine(box->x + 10, box->y + 27, box->x + 10 + MeasureText(klass->name, 16), box->y + 27, BLACK);
        }

        int text_y = y_name + 9;
        for (size_t a = 0; a < klass->attribute_count; a++) {
            char line[UML_TEXT_MAX];
            snprintf(line, sizeof(line), "%s %s: %s", visibility_symbol(klass->attributes[a].visibility), klass->attributes[a].name, klass->attributes[a].type);
            DrawText(line, box->x + 10, text_y, 14, BLACK);
            text_y += 20;
        }

        text_y = y_attrs + 9;
        for (size_t m = 0; m < klass->method_count; m++) {
            const MethodDecl *method = &klass->methods[m];
            char line[UML_TEXT_MAX * 2];
            int used = snprintf(line, sizeof(line), "%s %s(", visibility_symbol(method->visibility), method->name);
            for (size_t p = 0; p < method->param_count && used > 0 && (size_t)used < sizeof(line); p++) {
                used += snprintf(line + used, sizeof(line) - (size_t)used, "%s%s: %s", p == 0 ? "" : ", ", method->params[p].name, method->params[p].type);
            }
            if (used > 0 && (size_t)used < sizeof(line)) {
                snprintf(line + used, sizeof(line) - (size_t)used, "): %s", method->return_type);
            }
            DrawText(line, box->x + 10, text_y, 14, BLACK);
            text_y += 20;
        }
    }

    draw_relation_markers(diagram, layout);
}

bool render_raylib_view(const Diagram *diagram, const Layout *layout, const char *png_path, ErrorList *errors) {
    (void)errors;
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(layout->width, layout->height, "Mini PlantUML C");
    SetTargetFPS(60);

    bool screenshot_saved = false;
    while (!WindowShouldClose()) {
        BeginDrawing();
        draw_diagram_raylib(diagram, layout);
        EndDrawing();

        if (png_path != NULL && !screenshot_saved) {
            TakeScreenshot(png_path);
            screenshot_saved = true;
        }
    }

    CloseWindow();
    return true;
}

#else

bool render_raylib_view(const Diagram *diagram, const Layout *layout, const char *png_path, ErrorList *errors) {
    (void)diagram;
    (void)layout;
    (void)png_path;
    error_add(errors, 0, 0, "visualizacion nativa no disponible: compile con 'make raylib' e instale raylib");
    return false;
}

#endif
