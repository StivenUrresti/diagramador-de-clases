#include "uml.h"

#include <stdio.h>
#include <string.h>

static int max_int(int a, int b) {
    return a > b ? a : b;
}

static int text_width(const char *text) {
    return (int)strlen(text) * 8 + 34;
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

static int method_visual_line_count(const MethodDecl *method) {
    char line[UML_TEXT_MAX * 2];
    method_text(method, line, sizeof(line));
    return strlen(line) > 34 && method->param_count > 0 ? 2 : 1;
}

static int method_visual_width(const MethodDecl *method) {
    char line[UML_TEXT_MAX * 2];
    method_text(method, line, sizeof(line));
    if (strlen(line) <= 34 || method->param_count == 0) {
        return text_width(line);
    }

    char first[UML_TEXT_MAX];
    char second[UML_TEXT_MAX];
    snprintf(first, sizeof(first), "%s %s(", visibility_symbol(method->visibility), method->name);
    size_t used = 0;
    for (size_t i = 0; i < method->param_count && used < sizeof(second); i++) {
        used += (size_t)snprintf(second + used, sizeof(second) - used, "%s%s: %s", i == 0 ? "  " : ", ", method->params[i].name, method->params[i].type);
    }
    if (used < sizeof(second)) {
        snprintf(second + used, sizeof(second) - used, "): %s", method->return_type);
    }
    return max_int(text_width(first), text_width(second));
}

void compute_layout(const Diagram *diagram, Layout *layout) {
    memset(layout, 0, sizeof(*layout));
    const int margin = 40;
    const int col_gap = 90;
    const int row_gap = 80;
    int columns[UML_MAX_CLASSES];
    int rows[UML_MAX_CLASSES];
    int col_widths[UML_MAX_CLASSES];
    int row_heights[UML_MAX_CLASSES];
    size_t col_count = 0;
    size_t row_count = 0;

    layout->width = 700;
    layout->height = 500;

    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        const PositionDecl *position = diagram_find_position(diagram, klass->name);
        ClassBox *box = &layout->boxes[i];

        box->x = position != NULL ? position->x : margin;
        box->y = position != NULL ? position->y : margin;
        box->name_height = 28;
        box->attrs_height = max_int(26, (int)klass->attribute_count * 19 + 10);
        int method_lines = 0;
        for (size_t m = 0; m < klass->method_count; m++) {
            method_lines += method_visual_line_count(&klass->methods[m]);
        }
        box->methods_height = max_int(26, method_lines * 19 + 10);
        box->height = box->name_height + box->attrs_height + box->methods_height;
        box->width = max_int(160, text_width(klass->name));

        for (size_t a = 0; a < klass->attribute_count; a++) {
            char line[UML_TEXT_MAX];
            snprintf(line, sizeof(line), "%s %s: %s", visibility_symbol(klass->attributes[a].visibility), klass->attributes[a].name, klass->attributes[a].type);
            box->width = max_int(box->width, text_width(line));
        }

        for (size_t m = 0; m < klass->method_count; m++) {
            box->width = max_int(box->width, method_visual_width(&klass->methods[m]));
        }

        int source_x = position != NULL ? position->x : box->x;
        int source_y = position != NULL ? position->y : box->y;
        size_t col = 0;
        while (col < col_count && columns[col] != source_x) {
            col++;
        }
        if (col == col_count) {
            columns[col_count] = source_x;
            col_widths[col_count] = 0;
            col_count++;
        }
        size_t row = 0;
        while (row < row_count && rows[row] != source_y) {
            row++;
        }
        if (row == row_count) {
            rows[row_count] = source_y;
            row_heights[row_count] = 0;
            row_count++;
        }
        col_widths[col] = max_int(col_widths[col], box->width);
        row_heights[row] = max_int(row_heights[row], box->height);
    }

    for (size_t i = 0; i < col_count; i++) {
        for (size_t j = i + 1; j < col_count; j++) {
            if (columns[j] < columns[i]) {
                int tmp = columns[i];
                columns[i] = columns[j];
                columns[j] = tmp;
                tmp = col_widths[i];
                col_widths[i] = col_widths[j];
                col_widths[j] = tmp;
            }
        }
    }

    for (size_t i = 0; i < row_count; i++) {
        for (size_t j = i + 1; j < row_count; j++) {
            if (rows[j] < rows[i]) {
                int tmp = rows[i];
                rows[i] = rows[j];
                rows[j] = tmp;
                tmp = row_heights[i];
                row_heights[i] = row_heights[j];
                row_heights[j] = tmp;
            }
        }
    }

    for (size_t i = 0; i < diagram->class_count; i++) {
        const PositionDecl *position = diagram_find_position(diagram, diagram->classes[i].name);
        int source_x = position != NULL ? position->x : margin;
        int source_y = position != NULL ? position->y : margin;
        int x = margin;
        int y = margin;

        for (size_t col = 0; col < col_count; col++) {
            if (columns[col] == source_x) {
                layout->boxes[i].x = x;
                break;
            }
            x += col_widths[col] + col_gap;
        }

        for (size_t row = 0; row < row_count; row++) {
            if (rows[row] == source_y) {
                layout->boxes[i].y = y;
                break;
            }
            y += row_heights[row] + row_gap;
        }

        layout->width = max_int(layout->width, layout->boxes[i].x + layout->boxes[i].width + margin);
        layout->height = max_int(layout->height, layout->boxes[i].y + layout->boxes[i].height + margin);
    }
}

bool render_ascii(const Diagram *diagram, const Layout *layout) {
    (void)layout;
    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        printf("+------------------------------+\n");
        printf("| %-28s |\n", klass->name);
        printf("+------------------------------+\n");
        for (size_t a = 0; a < klass->attribute_count; a++) {
            printf("| %s %-12s: %-10s |\n",
                   visibility_symbol(klass->attributes[a].visibility),
                   klass->attributes[a].name,
                   klass->attributes[a].type);
        }
        printf("+------------------------------+\n");
        for (size_t m = 0; m < klass->method_count; m++) {
            char line[UML_TEXT_MAX * 2];
            method_text(&klass->methods[m], line, sizeof(line));
            printf("| %-28.28s |\n", line);
        }
        printf("+------------------------------+\n\n");
    }
    return true;
}
