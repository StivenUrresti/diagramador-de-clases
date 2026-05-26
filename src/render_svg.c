#include "uml.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void xml_text(FILE *out, const char *text) {
    for (const char *p = text; *p; p++) {
        switch (*p) {
            case '&':
                fputs("&amp;", out);
                break;
            case '<':
                fputs("&lt;", out);
                break;
            case '>':
                fputs("&gt;", out);
                break;
            case '"':
                fputs("&quot;", out);
                break;
            default:
                fputc(*p, out);
                break;
        }
    }
}

static void draw_marker(FILE *out, RelationType type, Point from, Point to) {
    double dx = (double)(to.x - from.x);
    double dy = (double)(to.y - from.y);
    double length = sqrt(dx * dx + dy * dy);
    if (length < 1.0) {
        return;
    }

    double ux = dx / length;
    double uy = dy / length;
    double px = -uy;
    double py = ux;

    if (type == REL_HERENCIA) {
        Point a = {(int)(to.x - ux * 18 + px * 9), (int)(to.y - uy * 18 + py * 9)};
        Point b = {(int)(to.x - ux * 18 - px * 9), (int)(to.y - uy * 18 - py * 9)};
        fprintf(out, "<polygon points=\"%d,%d %d,%d %d,%d\" fill=\"white\" stroke=\"black\" stroke-width=\"2\"/>\n", to.x, to.y, a.x, a.y, b.x, b.y);
    } else if (type == REL_AGREGACION || type == REL_COMPOSICION) {
        Point a = {(int)(to.x - ux * 13 + px * 8), (int)(to.y - uy * 13 + py * 8)};
        Point b = {(int)(to.x - ux * 26), (int)(to.y - uy * 26)};
        Point c = {(int)(to.x - ux * 13 - px * 8), (int)(to.y - uy * 13 - py * 8)};
        fprintf(out, "<polygon points=\"%d,%d %d,%d %d,%d %d,%d\" fill=\"%s\" stroke=\"black\" stroke-width=\"2\"/>\n",
                to.x, to.y, a.x, a.y, b.x, b.y, c.x, c.y, type == REL_COMPOSICION ? "black" : "white");
    } else if (type == REL_DEPENDENCIA) {
        Point a = {(int)(to.x - ux * 14 + px * 7), (int)(to.y - uy * 14 + py * 7)};
        Point b = {(int)(to.x - ux * 14 - px * 7), (int)(to.y - uy * 14 - py * 7)};
        fprintf(out, "<polyline points=\"%d,%d %d,%d %d,%d\" fill=\"none\" stroke=\"black\" stroke-width=\"2\"/>\n", a.x, a.y, to.x, to.y, b.x, b.y);
    }
}

static void method_text(const MethodDecl *method, char *buffer, size_t size) {
    size_t used = (size_t)snprintf(buffer, size, "%s %s(", visibility_symbol(method->visibility), method->name);
    for (size_t i = 0; i < method->param_count && used < size; i++) {
        used += (size_t)snprintf(buffer + used, size - used, "%s%s: %s", i == 0 ? "" : ", ", method->params[i].name, method->params[i].type);
    }
    if (used < size) {
        snprintf(buffer + used, size - used, "): %s", method->return_type);
    }
}

static void draw_method_text(FILE *out, const MethodDecl *method, int x, int *y) {
    char line[UML_TEXT_MAX * 2];
    method_text(method, line, sizeof(line));
    if (strlen(line) <= 34 || method->param_count == 0) {
        fprintf(out, "<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"13\">", x, *y);
        xml_text(out, line);
        fprintf(out, "</text>\n");
        *y += 19;
        return;
    }

    fprintf(out, "<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"13\">%s ",
            x, *y, visibility_symbol(method->visibility));
    xml_text(out, method->name);
    fprintf(out, "(</text>\n");
    *y += 19;

    fprintf(out, "<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"13\">", x + 14, *y);
    for (size_t p = 0; p < method->param_count; p++) {
        if (p > 0) {
            fprintf(out, ", ");
        }
        xml_text(out, method->params[p].name);
        fprintf(out, ": ");
        xml_text(out, method->params[p].type);
    }
    fprintf(out, "): ");
    xml_text(out, method->return_type);
    fprintf(out, "</text>\n");
    *y += 19;
}

static void draw_relation_lines(FILE *out, const Diagram *diagram, const Layout *layout) {
    for (size_t r = 0; r < diagram->relation_count; r++) {
        const RelationDecl *relation = &diagram->relations[r];
        Point points[UML_MAX_POINTS];
        size_t count = compute_relation_points(diagram, layout, relation, points, UML_MAX_POINTS);
        if (count < 2) {
            continue;
        }

        fprintf(out, "<polyline points=\"");
        for (size_t i = 0; i < count; i++) {
            fprintf(out, "%s%d,%d", i == 0 ? "" : " ", points[i].x, points[i].y);
        }
        fprintf(out, "\" fill=\"none\" stroke=\"black\" stroke-width=\"2\"");
        if (relation->type == REL_DEPENDENCIA) {
            fprintf(out, " stroke-dasharray=\"7 5\"");
        }
        fprintf(out, "/>\n");
    }
}

static void draw_relation_markers(FILE *out, const Diagram *diagram, const Layout *layout) {
    for (size_t r = 0; r < diagram->relation_count; r++) {
        const RelationDecl *relation = &diagram->relations[r];
        Point points[UML_MAX_POINTS];
        size_t count = compute_relation_points(diagram, layout, relation, points, UML_MAX_POINTS);
        if (count < 2) {
            continue;
        }
        draw_marker(out, relation->type, points[count - 2], points[count - 1]);
    }
}

static void draw_classes(FILE *out, const Diagram *diagram, const Layout *layout) {
    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        const ClassBox *box = &layout->boxes[i];
        int y_name = box->y + box->name_height;
        int y_attrs = y_name + box->attrs_height;

        fprintf(out, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"white\" stroke=\"black\" stroke-width=\"2\"/>\n", box->x, box->y, box->width, box->height);
        fprintf(out, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"black\" stroke-width=\"1\"/>\n", box->x, y_name, box->x + box->width, y_name);
        fprintf(out, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" stroke=\"black\" stroke-width=\"1\"/>\n", box->x, y_attrs, box->x + box->width, y_attrs);

        fprintf(out, "<text x=\"%d\" y=\"%d\" text-anchor=\"middle\" font-family=\"monospace\" font-size=\"15\"%s>",
                box->x + box->width / 2, box->y + 21, klass->is_abstract ? " font-style=\"italic\"" : "");
        xml_text(out, klass->name);
        fprintf(out, "</text>\n");

        int text_y = y_name + 18;
        for (size_t a = 0; a < klass->attribute_count; a++) {
            fprintf(out, "<text x=\"%d\" y=\"%d\" font-family=\"monospace\" font-size=\"13\">", box->x + 10, text_y);
            fprintf(out, "%s ", visibility_symbol(klass->attributes[a].visibility));
            xml_text(out, klass->attributes[a].name);
            fprintf(out, ": ");
            xml_text(out, klass->attributes[a].type);
            fprintf(out, "</text>\n");
            text_y += 19;
        }

        text_y = y_attrs + 18;
        for (size_t m = 0; m < klass->method_count; m++) {
            draw_method_text(out, &klass->methods[m], box->x + 10, &text_y);
        }
    }
}

bool render_svg(const Diagram *diagram, const Layout *layout, const char *path, ErrorList *errors) {
    FILE *out = fopen(path, "w");
    if (out == NULL) {
        error_add(errors, 0, 0, "no se pudo crear SVG '%s'", path);
        return false;
    }

    fprintf(out, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(out, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\">\n",
            layout->width, layout->height, layout->width, layout->height);
    fprintf(out, "<rect width=\"100%%\" height=\"100%%\" fill=\"#f7f7f7\"/>\n");
    fprintf(out, "<text x=\"20\" y=\"30\" font-family=\"monospace\" font-size=\"18\">");
    xml_text(out, diagram->name);
    fprintf(out, "</text>\n");

    draw_relation_lines(out, diagram, layout);
    draw_classes(out, diagram, layout);
    draw_relation_markers(out, diagram, layout);

    fprintf(out, "</svg>\n");
    fclose(out);
    return true;
}
