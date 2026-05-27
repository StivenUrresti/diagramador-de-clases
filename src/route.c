#include "uml.h"

#include <limits.h>
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

static bool segment_crosses_box(int x1, int y1, int x2, int y2, const ClassBox *box, int pad) {
    int left = box->x + pad;
    int right = box->x + box->width - pad;
    int top = box->y + pad;
    int bottom = box->y + box->height - pad;

    if (y1 == y2) {
        if (y1 <= top || y1 >= bottom) {
            return false;
        }
        int minx = x1 < x2 ? x1 : x2;
        int maxx = x1 > x2 ? x1 : x2;
        return maxx > left && minx < right;
    }

    if (x1 == x2) {
        if (x1 <= left || x1 >= right) {
            return false;
        }
        int miny = y1 < y2 ? y1 : y2;
        int maxy = y1 > y2 ? y1 : y2;
        return maxy > top && miny < bottom;
    }

    return false;
}

static bool path_hits_boxes_count(const Point *points, size_t count, const Layout *layout, size_t box_count, int from_idx, int to_idx) {
    for (size_t i = 1; i < count; i++) {
        for (size_t bi = 0; bi < box_count; bi++) {
            const ClassBox *box = &layout->boxes[bi];
            if (box->width <= 0 || box->height <= 0) {
                continue;
            }
            if ((int)bi == from_idx || (int)bi == to_idx) {
                continue;
            }
            if (segment_crosses_box(points[i - 1].x, points[i - 1].y, points[i].x, points[i].y, box, 2)) {
                return true;
            }
        }
    }
    return false;
}

static void pick_anchors(const ClassBox *from, const ClassBox *to, Point *start, Point *end) {
    int fcx = from->x + from->width / 2;
    int fcy = from->y + from->height / 2;
    int tcx = to->x + to->width / 2;
    int tcy = to->y + to->height / 2;
    int dx = tcx - fcx;
    int dy = tcy - fcy;

    if (dy < -16) {
        *start = (Point){fcx, from->y};
        *end = (Point){tcx, to->y + to->height};
    } else if (dy > 16) {
        *start = (Point){fcx, from->y + from->height};
        *end = (Point){tcx, to->y};
    } else if (dx > 0) {
        *start = (Point){from->x + from->width, fcy};
        *end = (Point){to->x, tcy};
    } else {
        *start = (Point){from->x, fcy};
        *end = (Point){to->x + to->width, tcy};
    }
}

static void append_hvh(Point *points, size_t *count, size_t max_points, Point start, Point end, int mid_y) {
    *count = 0;
    add_point(points, count, max_points, start);
    if (start.y != mid_y) {
        add_point(points, count, max_points, (Point){start.x, mid_y});
    }
    if (end.x != start.x) {
        add_point(points, count, max_points, (Point){end.x, mid_y});
    }
    if (end.y != mid_y || end.x != start.x) {
        add_point(points, count, max_points, end);
    }
}

static void append_lvl(Point *points, size_t *count, size_t max_points, Point start, Point end, int mid_x) {
    *count = 0;
    add_point(points, count, max_points, start);
    if (start.x != mid_x) {
        add_point(points, count, max_points, (Point){mid_x, start.y});
    }
    if (end.y != start.y) {
        add_point(points, count, max_points, (Point){mid_x, end.y});
    }
    if (end.x != mid_x || end.y != start.y) {
        add_point(points, count, max_points, end);
    }
}

static int min_box_left(const Layout *layout, size_t box_count) {
    int value = layout->boxes[0].x;
    for (size_t i = 1; i < box_count; i++) {
        if (layout->boxes[i].x < value) {
            value = layout->boxes[i].x;
        }
    }
    return value;
}

static int max_box_right(const Layout *layout, size_t box_count) {
    const ClassBox *box = &layout->boxes[0];
    int value = box->x + box->width;
    for (size_t i = 1; i < box_count; i++) {
        box = &layout->boxes[i];
        int right = box->x + box->width;
        if (right > value) {
            value = right;
        }
    }
    return value;
}

static bool boxes_share_column(const ClassBox *from, const ClassBox *to) {
    int left = from->x > to->x ? to->x : from->x;
    int right_from = from->x + from->width;
    int right_to = to->x + to->width;
    int right = right_from < right_to ? right_to : right_from;
    return (right - left) <= (from->width + to->width) / 2 + 20;
}

static bool channel_y_taken(const int *used, size_t used_count, int y) {
    for (size_t i = 0; i < used_count; i++) {
        if (abs(used[i] - y) < 12) {
            return true;
        }
    }
    return false;
}

static void remember_channel_y(int *used, size_t *used_count, size_t max_used, int y) {
    if (*used_count < max_used && !channel_y_taken(used, *used_count, y)) {
        used[(*used_count)++] = y;
    }
}

static void route_same_column(const ClassBox *from, const ClassBox *to, int lane, Point *points, size_t *count) {
    int aligned = abs(from->x - to->x) <= 8;
    int from_cx = from->x + from->width / 2;
    int to_cx = to->x + to->width / 2;
    int spine = aligned ? ((from_cx + to_cx) / 2) : from->x + 18 + (lane % 3) * 10;
    Point start;
    Point end;

    *count = 0;
    if (aligned) {
        if (to->y + to->height <= from->y) {
            start = (Point){spine, from->y};
            end = (Point){spine, to->y + to->height};
        } else {
            start = (Point){spine, from->y + from->height};
            end = (Point){spine, to->y};
        }
        add_point(points, count, UML_MAX_POINTS, start);
        add_point(points, count, UML_MAX_POINTS, end);
        return;
    }

    if (to->y + to->height <= from->y) {
        start = (Point){from_cx, from->y};
        end = (Point){to_cx, to->y + to->height};
    } else {
        start = (Point){from_cx, from->y + from->height};
        end = (Point){to_cx, to->y};
    }

    add_point(points, count, UML_MAX_POINTS, start);
    if (spine != start.x) {
        add_point(points, count, UML_MAX_POINTS, (Point){spine, start.y});
    }
    if (spine != end.x || start.y != end.y) {
        add_point(points, count, UML_MAX_POINTS, (Point){spine, end.y});
    }
    if (end.x != spine || end.y != start.y) {
        add_point(points, count, UML_MAX_POINTS, end);
    }
}

static void append_hv_h_margin(const ClassBox *from, const ClassBox *to, int margin_x, Point *points, size_t *count) {
    Point start = {from->x + from->width / 2, from->y + from->height};
    Point end = {to->x + to->width / 2, to->y};
    *count = 0;
    add_point(points, count, UML_MAX_POINTS, start);
    add_point(points, count, UML_MAX_POINTS, (Point){margin_x, start.y});
    add_point(points, count, UML_MAX_POINTS, (Point){margin_x, end.y});
    add_point(points, count, UML_MAX_POINTS, end);
}

static void copy_route(Point *dest, size_t *dest_count, const Point *src, size_t src_count) {
    *dest_count = src_count;
    for (size_t i = 0; i < src_count; i++) {
        dest[i] = src[i];
    }
}

static void simplify_path(Point *points, size_t *count) {
    if (*count <= 2) {
        return;
    }

    Point simplified[UML_MAX_POINTS];
    size_t out = 0;
    simplified[out++] = points[0];

    for (size_t i = 1; i + 1 < *count; i++) {
        Point prev = simplified[out - 1];
        Point curr = points[i];
        Point next = points[i + 1];
        bool collinear = (prev.x == curr.x && curr.x == next.x) || (prev.y == curr.y && curr.y == next.y);
        if (!collinear) {
            simplified[out++] = curr;
        }
    }

    simplified[out++] = points[*count - 1];
    copy_route(points, count, simplified, out);
}

static int path_score(const Point *points, size_t count, const int *used_channels, size_t used_channel_count) {
    if (count < 2) {
        return INT_MAX;
    }

    int length = 0;
    int bends = 0;
    for (size_t i = 1; i < count; i++) {
        length += abs(points[i].x - points[i - 1].x) + abs(points[i].y - points[i - 1].y);
        if (i >= 2) {
            Point a = points[i - 2];
            Point b = points[i - 1];
            Point c = points[i];
            bool ab_horiz = a.y == b.y;
            bool bc_horiz = b.y == c.y;
            if (ab_horiz != bc_horiz) {
                bends++;
            }
        }
        if (points[i - 1].y == points[i].y && channel_y_taken(used_channels, used_channel_count, points[i].y)) {
            length += 24;
        }
    }

    return length + bends * 10;
}

static bool consider_route(
    const Point *candidate,
    size_t candidate_count,
    const Layout *layout,
    size_t box_count,
    int from_idx,
    int to_idx,
    const int *used_channels,
    size_t used_channel_count,
    Point *best,
    size_t *best_count,
    int *best_score) {
    Point simplified[UML_MAX_POINTS];
    size_t simplified_count = candidate_count;

    if (candidate_count < 2 || candidate_count > UML_MAX_POINTS) {
        return false;
    }

    for (size_t i = 0; i < candidate_count; i++) {
        simplified[i] = candidate[i];
    }
    simplify_path(simplified, &simplified_count);
    if (simplified_count < 2) {
        return false;
    }
    if (path_hits_boxes_count(simplified, simplified_count, layout, box_count, from_idx, to_idx)) {
        return false;
    }

    int score = path_score(simplified, simplified_count, used_channels, used_channel_count);
    if (*best_count == 0 || score < *best_score) {
        copy_route(best, best_count, simplified, simplified_count);
        *best_score = score;
        return true;
    }

    return false;
}

static void route_same_row_direct(const ClassBox *from, const ClassBox *to, Point *points, size_t *count) {
    int y = (from->y + from->height / 2 + to->y + to->height / 2) / 2;
    Point start;
    Point end;

    *count = 0;
    if (from->x + from->width <= to->x) {
        start = (Point){from->x + from->width, y};
        end = (Point){to->x, y};
    } else if (to->x + to->width <= from->x) {
        start = (Point){from->x, y};
        end = (Point){to->x + to->width, y};
    } else {
        return;
    }

    add_point(points, count, UML_MAX_POINTS, start);
    add_point(points, count, UML_MAX_POINTS, end);
}

static void route_below_left_corridor(const ClassBox *from, const ClassBox *to, int lane, Point *points, size_t *count) {
    int corridor = to->x + to->width + ((from->x - (to->x + to->width)) / 2) + (lane % 3) * 5;
    Point start;
    Point end = {to->x + to->width / 2, to->y};

    *count = 0;
    if (to->y >= from->y + from->height) {
        start = (Point){from->x + from->width / 2, from->y + from->height};
    } else {
        start = (Point){from->x, from->y + from->height / 2};
    }

    add_point(points, count, UML_MAX_POINTS, start);
    if (start.x != corridor) {
        add_point(points, count, UML_MAX_POINTS, (Point){corridor, start.y});
    }
    if (corridor != end.x || start.y != end.y) {
        add_point(points, count, UML_MAX_POINTS, (Point){corridor, end.y});
    }
    add_point(points, count, UML_MAX_POINTS, end);
}

static void route_side_corridor(const ClassBox *from, const ClassBox *to, int lane, int use_right_margin, const Layout *layout, size_t box_count, Point *points, size_t *count) {
    int corridor = use_right_margin
        ? max_box_right(layout, box_count) + 18 + lane
        : min_box_left(layout, box_count) - 18 - lane;
    Point start;
    Point end;

    pick_anchors(from, to, &start, &end);
    append_lvl(points, count, UML_MAX_POINTS, start, end, corridor);
}

static void build_route(const ClassBox *from, const ClassBox *to, const Layout *layout, size_t box_count, int from_idx, int to_idx, int lane, const int *used_channels, size_t used_channel_count, Point *points, size_t *count) {
    Point start;
    Point end;
    int fcx = from->x + from->width / 2;
    int fcy = from->y + from->height / 2;
    int tcx = to->x + to->width / 2;
    int tcy = to->y + to->height / 2;
    int dx = tcx - fcx;
    int dy = tcy - fcy;

    Point candidate[UML_MAX_POINTS];
    size_t candidate_count = 0;
    Point best[UML_MAX_POINTS];
    size_t best_count = 0;
    int best_score = 0;
    int y_options[6];
    int x_options[4];
    size_t y_option_count = 0;
    size_t x_option_count = 0;

    if (boxes_share_column(from, to)) {
        route_same_column(from, to, lane, points, count);
        simplify_path(points, count);
        return;
    }

    if (abs(dy) <= 24 && abs(dx) > 24) {
        route_same_row_direct(from, to, candidate, &candidate_count);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);

        int below = (from->y + from->height > to->y + to->height ? from->y + from->height : to->y + to->height) + 16 + lane;
        int above = (from->y < to->y ? from->y : to->y) - 16 - lane;
        if (channel_y_taken(used_channels, used_channel_count, below)) {
            below += 18;
        }
        if (channel_y_taken(used_channels, used_channel_count, above)) {
            above -= 18;
        }

        if (dx < 0) {
            start = (Point){from->x, fcy};
            end = (Point){to->x + to->width, tcy};
        } else {
            start = (Point){from->x + from->width, fcy};
            end = (Point){to->x, tcy};
        }

        append_hvh(candidate, &candidate_count, UML_MAX_POINTS, start, end, above);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);

        append_hvh(candidate, &candidate_count, UML_MAX_POINTS, start, end, below);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);
    }

    if (to->x + to->width < from->x && to->y >= from->y + from->height / 2) {
        route_below_left_corridor(from, to, lane, candidate, &candidate_count);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);
    }

    if (to->x + to->width < from->x && to->y + to->height <= from->y) {
        append_hv_h_margin(from, to, max_box_right(layout, box_count) + 18 + lane, candidate, &candidate_count);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);
    }

    pick_anchors(from, to, &start, &end);

    y_options[y_option_count++] = (start.y + end.y) / 2;
    y_options[y_option_count++] = from->y + from->height + 14 + lane;
    y_options[y_option_count++] = to->y + to->height + 14 + lane;
    y_options[y_option_count++] = from->y - 14 - lane;
    y_options[y_option_count++] = to->y - 14 - lane;
    y_options[y_option_count++] = start.y + ((end.y - start.y) / 3) + (lane / 2);

    x_options[x_option_count++] = min_box_left(layout, box_count) - 16 - lane;
    x_options[x_option_count++] = max_box_right(layout, box_count) + 16 + lane;
    x_options[x_option_count++] = layout->width - 20 - lane;
    x_options[x_option_count++] = 12 + lane;

    for (size_t yi = 0; yi < y_option_count; yi++) {
        if (channel_y_taken(used_channels, used_channel_count, y_options[yi])) {
            continue;
        }
        append_hvh(candidate, &candidate_count, UML_MAX_POINTS, start, end, y_options[yi]);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);
    }

    if (to->x + to->width < from->x || from->x + from->width < to->x) {
        route_side_corridor(from, to, lane, to->x > from->x, layout, box_count, candidate, &candidate_count);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);
    }

    for (size_t xi = 0; xi < x_option_count; xi++) {
        append_lvl(candidate, &candidate_count, UML_MAX_POINTS, start, end, x_options[xi]);
        consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);
    }

    append_hvh(candidate, &candidate_count, UML_MAX_POINTS, start, end, (start.y + end.y) / 2);
    consider_route(candidate, candidate_count, layout, box_count, from_idx, to_idx, used_channels, used_channel_count, best, &best_count, &best_score);

    if (best_count >= 2) {
        copy_route(points, count, best, best_count);
        return;
    }

    append_hvh(candidate, &candidate_count, UML_MAX_POINTS, start, end, (start.y + end.y) / 2);
    copy_route(points, count, candidate, candidate_count);
}

void plan_routes(Diagram *diagram, const Layout *layout) {
    size_t box_count = diagram->class_count;
    int used_channels[UML_MAX_RELATIONS];
    size_t used_channel_count = 0;

    for (size_t r = 0; r < diagram->relation_count; r++) {
        RelationDecl *relation = &diagram->relations[r];
        int from_idx = class_index(diagram, relation->from);
        int to_idx = class_index(diagram, relation->to);
        if (from_idx < 0 || to_idx < 0) {
            relation->point_count = 0;
            continue;
        }

        Point planned[UML_MAX_POINTS];
        size_t planned_count = 0;
        build_route(&layout->boxes[from_idx], &layout->boxes[to_idx], layout, box_count, from_idx, to_idx, (int)r * 14, used_channels, used_channel_count, planned, &planned_count);

        relation->point_count = planned_count;
        for (size_t i = 0; i < planned_count; i++) {
            relation->points[i] = planned[i];
        }

        if (planned_count >= 2) {
            for (size_t i = 1; i < planned_count; i++) {
                if (planned[i - 1].y == planned[i].y) {
                    remember_channel_y(used_channels, &used_channel_count, UML_MAX_RELATIONS, planned[i - 1].y);
                }
            }
        }
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
