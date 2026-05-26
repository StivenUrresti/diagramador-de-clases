#include "uml.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void error_list_init(ErrorList *errors) {
    errors->count = 0;
}

void error_add(ErrorList *errors, int line, int column, const char *fmt, ...) {
    if (errors->count >= UML_MAX_ERRORS) {
        return;
    }

    UmlError *error = &errors->items[errors->count++];
    error->line = line;
    error->column = column;

    va_list args;
    va_start(args, fmt);
    vsnprintf(error->message, sizeof(error->message), fmt, args);
    va_end(args);
}

void error_print_all(const ErrorList *errors) {
    for (size_t i = 0; i < errors->count; i++) {
        const UmlError *error = &errors->items[i];
        if (error->line > 0) {
            fprintf(stderr, "Error %d:%d: %s\n", error->line, error->column, error->message);
        } else {
            fprintf(stderr, "Error: %s\n", error->message);
        }
    }
}

void diagram_init(Diagram *diagram) {
    memset(diagram, 0, sizeof(*diagram));
}

const ClassDecl *diagram_find_class(const Diagram *diagram, const char *name) {
    for (size_t i = 0; i < diagram->class_count; i++) {
        if (strcmp(diagram->classes[i].name, name) == 0) {
            return &diagram->classes[i];
        }
    }
    return NULL;
}

const PositionDecl *diagram_find_position(const Diagram *diagram, const char *name) {
    for (size_t i = 0; i < diagram->position_count; i++) {
        if (strcmp(diagram->positions[i].name, name) == 0) {
            return &diagram->positions[i];
        }
    }
    return NULL;
}

const char *relation_name(RelationType type) {
    switch (type) {
        case REL_HERENCIA:
            return "herencia";
        case REL_DEPENDENCIA:
            return "dependencia";
        case REL_COMPOSICION:
            return "composicion";
        case REL_AGREGACION:
            return "agregacion";
        case REL_NONE:
        default:
            return "ninguna";
    }
}

const char *visibility_symbol(Visibility visibility) {
    switch (visibility) {
        case VIS_PUBLICO:
            return "+";
        case VIS_PRIVADO:
            return "-";
        case VIS_PROTEGIDO:
            return "#";
        default:
            return "?";
    }
}
