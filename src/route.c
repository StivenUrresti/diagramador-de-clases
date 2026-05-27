#include "uml.h"

#include <stdlib.h>
#include <string.h>

static int class_index(const Diagram *diagram, const char *name) {
    for (size_t i = 0; i < diagram->class_count; i++) {
        if (strcmp(diagram->classes[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static Point box_center(const ClassBox *box) {
    Point point = {box->x + box->width / 2, box->y + box->height / 2};
    return point;
}

static Point edge_point(const ClassBox *box, int horizontal, int positive) {
    Point center = box_center(box);
    if (horizontal) {
        center.x = positive ? box->x + box->width : box->x;
    } else {
        center.y = positive ? box->y + box->height : box->y;
    }
    return center;
}

static void add_point(Point *points, size_t *count, size_t max_points, Point point) {
    if (*count > 0) {
        Point previous = points[*count - 1];
        if (previous.x == point.x && previous.y == point.y) {
            return;
        }
    }

    if (*count < max_points) {
        points[(*count)++] = point;
    }
}

size_t compute_relation_points(const Diagram *diagram, const Layout *layout, const RelationDecl *relation, Point *points, size_t max_points) {
    if (relation->point_count >= 2 && max_points > 0) {
        size_t count = relation->point_count;
        if (count > max_points) {
            count = max_points;
        }
        for (size_t i = 0; i < count; i++) {
            points[i] = relation->points[i];
        }
        return count;
    }

    int from = class_index(diagram, relation->from);
    int to = class_index(diagram, relation->to);
    if (from < 0 || to < 0 || max_points == 0) {
        return 0;
    }

    const ClassBox *from_box = &layout->boxes[from];
    const ClassBox *to_box = &layout->boxes[to];
    Point from_center = box_center(from_box);
    Point to_center = box_center(to_box);
    int dx = to_center.x - from_center.x;
    int dy = to_center.y - from_center.y;
    size_t count = 0;

    if (abs(dx) > abs(dy)) {
        Point start = edge_point(from_box, 1, dx > 0);
        Point end = edge_point(to_box, 1, dx < 0);
        int mid_x = (start.x + end.x) / 2;
        add_point(points, &count, max_points, start);
        add_point(points, &count, max_points, (Point){mid_x, start.y});
        add_point(points, &count, max_points, (Point){mid_x, end.y});
        add_point(points, &count, max_points, end);
    } else {
        Point start = edge_point(from_box, 0, dy > 0);
        Point end = edge_point(to_box, 0, dy < 0);
        int mid_y = (start.y + end.y) / 2;
        add_point(points, &count, max_points, start);
        add_point(points, &count, max_points, (Point){start.x, mid_y});
        add_point(points, &count, max_points, (Point){end.x, mid_y});
        add_point(points, &count, max_points, end);
    }

    return count;
}
