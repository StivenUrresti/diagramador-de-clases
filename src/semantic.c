#include "uml.h"

#include <stdio.h>
#include <string.h>

static int class_index(const Diagram *diagram, const char *name) {
    for (size_t i = 0; i < diagram->class_count; i++) {
        if (strcmp(diagram->classes[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static const AttributeDecl *find_attribute_direct(const ClassDecl *klass, const char *name) {
    for (size_t i = 0; i < klass->attribute_count; i++) {
        if (strcmp(klass->attributes[i].name, name) == 0) {
            return &klass->attributes[i];
        }
    }
    return NULL;
}

static bool method_same_signature(const MethodDecl *a, const MethodDecl *b) {
    if (strcmp(a->name, b->name) != 0 || a->param_count != b->param_count) {
        return false;
    }
    for (size_t i = 0; i < a->param_count; i++) {
        if (strcmp(a->params[i].type, b->params[i].type) != 0) {
            return false;
        }
    }
    return true;
}

static bool inherited_attribute_exists(const Diagram *diagram, const ClassDecl *klass, const char *attribute_name, int depth) {
    if (depth > UML_MAX_CLASSES || klass->relation != REL_HERENCIA || klass->relation_target[0] == '\0') {
        return false;
    }

    int parent_idx = class_index(diagram, klass->relation_target);
    if (parent_idx < 0) {
        return false;
    }

    const ClassDecl *parent = &diagram->classes[parent_idx];
    if (find_attribute_direct(parent, attribute_name) != NULL) {
        return true;
    }

    return inherited_attribute_exists(diagram, parent, attribute_name, depth + 1);
}

static void copy_segment_points(RelationDecl *relation, const SegmentDecl *segment, bool reverse) {
    relation->point_count = segment->point_count;
    for (size_t i = 0; i < segment->point_count; i++) {
        size_t source_index = reverse ? segment->point_count - i - 1 : i;
        relation->points[i] = segment->points[source_index];
    }
}

static const SegmentDecl *find_segment_for_relation(const Diagram *diagram, const char *from, const char *to, RelationType type, bool *reverse) {
    for (size_t i = 0; i < diagram->segment_count; i++) {
        const SegmentDecl *segment = &diagram->segments[i];
        if (segment->relation != type) {
            continue;
        }
        if (strcmp(segment->from, from) == 0 && strcmp(segment->to, to) == 0) {
            *reverse = false;
            return segment;
        }
        if (strcmp(segment->from, to) == 0 && strcmp(segment->to, from) == 0) {
            *reverse = true;
            return segment;
        }
    }
    return NULL;
}

static bool add_relation(Diagram *diagram, const char *from, const char *to, RelationType type) {
    if (diagram->relation_count >= UML_MAX_RELATIONS) {
        return false;
    }

    RelationDecl *relation = &diagram->relations[diagram->relation_count++];
    memset(relation, 0, sizeof(*relation));
    snprintf(relation->from, sizeof(relation->from), "%s", from);
    snprintf(relation->to, sizeof(relation->to), "%s", to);
    relation->type = type;

    bool reverse = false;
    const SegmentDecl *segment = find_segment_for_relation(diagram, from, to, type, &reverse);
    if (segment != NULL) {
        copy_segment_points(relation, segment, reverse);
    }

    return true;
}

bool analyze_semantics(Diagram *diagram, ErrorList *errors) {
    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];

        for (size_t j = i + 1; j < diagram->class_count; j++) {
            if (strcmp(klass->name, diagram->classes[j].name) == 0) {
                error_add(errors, diagram->classes[j].line, diagram->classes[j].column, "clase duplicada '%s'", klass->name);
            }
        }

        if (diagram_find_position(diagram, klass->name) == NULL) {
            error_add(errors, klass->line, klass->column, "la clase '%s' no tiene posicion declarada", klass->name);
        }
    }

    for (size_t i = 0; i < diagram->position_count; i++) {
        const PositionDecl *position = &diagram->positions[i];
        for (size_t j = i + 1; j < diagram->position_count; j++) {
            const PositionDecl *other = &diagram->positions[j];
            if (strcmp(position->name, other->name) == 0) {
                error_add(errors, other->line, other->column, "posicion duplicada para la clase '%s'", other->name);
            }
            if (position->x == other->x && position->y == other->y) {
                error_add(errors, other->line, other->column, "dos clases no pueden ocupar la misma posicion (%d,%d)", other->x, other->y);
            }
        }
        if (diagram_find_class(diagram, position->name) == NULL) {
            error_add(errors, position->line, position->column, "posicion declarada para clase inexistente '%s'", position->name);
        }
    }

    for (size_t i = 0; i < diagram->segment_count; i++) {
        const SegmentDecl *segment = &diagram->segments[i];
        if (diagram_find_class(diagram, segment->from) == NULL) {
            error_add(errors, segment->line, segment->column, "segmento referencia clase origen inexistente '%s'", segment->from);
        }
        if (diagram_find_class(diagram, segment->to) == NULL) {
            error_add(errors, segment->line, segment->column, "segmento referencia clase destino inexistente '%s'", segment->to);
        }
        if (segment->point_count < 2) {
            error_add(errors, segment->line, segment->column, "segmento entre '%s' y '%s' debe tener al menos dos puntos", segment->from, segment->to);
        }
    }

    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        if (klass->relation != REL_NONE && diagram_find_class(diagram, klass->relation_target) == NULL) {
            error_add(errors, klass->line, klass->column, "la relacion de '%s' apunta a clase inexistente '%s'", klass->name, klass->relation_target);
        }

        for (size_t a = 0; a < klass->attribute_count; a++) {
            const AttributeDecl *attribute = &klass->attributes[a];
            for (size_t b = a + 1; b < klass->attribute_count; b++) {
                if (strcmp(attribute->name, klass->attributes[b].name) == 0) {
                    error_add(errors, klass->attributes[b].line, klass->attributes[b].column, "atributo duplicado '%s' en clase '%s'", attribute->name, klass->name);
                }
            }
            if (inherited_attribute_exists(diagram, klass, attribute->name, 0)) {
                error_add(errors, attribute->line, attribute->column, "atributo '%s' ya existe en la herencia de '%s'", attribute->name, klass->name);
            }
        }

        for (size_t a = 0; a < klass->method_count; a++) {
            for (size_t attr = 0; attr < klass->attribute_count; attr++) {
                if (strcmp(klass->methods[a].name, klass->attributes[attr].name) == 0) {
                    error_add(errors, klass->methods[a].line, klass->methods[a].column, "metodo '%s' no puede llamarse igual que un atributo en '%s'", klass->methods[a].name, klass->name);
                    break;
                }
            }
            for (size_t b = a + 1; b < klass->method_count; b++) {
                if (method_same_signature(&klass->methods[a], &klass->methods[b])) {
                    error_add(errors, klass->methods[b].line, klass->methods[b].column, "metodo duplicado '%s' con la misma firma en clase '%s'", klass->methods[b].name, klass->name);
                }
            }
        }
    }

    if (errors->count > 0) {
        return false;
    }

    diagram->relation_count = 0;
    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        if (klass->relation != REL_NONE && !add_relation(diagram, klass->name, klass->relation_target, klass->relation)) {
            error_add(errors, klass->line, klass->column, "demasiadas relaciones en el diagrama");
            return false;
        }
    }

    for (size_t i = 0; i < diagram->segment_count; i++) {
        const SegmentDecl *segment = &diagram->segments[i];
        bool already_defined = false;
        for (size_t r = 0; r < diagram->relation_count; r++) {
            const RelationDecl *relation = &diagram->relations[r];
            if (relation->type == segment->relation &&
                ((strcmp(relation->from, segment->from) == 0 && strcmp(relation->to, segment->to) == 0) ||
                 (strcmp(relation->from, segment->to) == 0 && strcmp(relation->to, segment->from) == 0))) {
                already_defined = true;
                break;
            }
        }
        if (!already_defined && !add_relation(diagram, segment->from, segment->to, segment->relation)) {
            error_add(errors, segment->line, segment->column, "demasiadas relaciones en el diagrama");
            return false;
        }
    }

    return true;
}
