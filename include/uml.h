#ifndef UML_H
#define UML_H

#include <stdbool.h>
#include <stddef.h>

#define UML_MAX_CLASSES 128
#define UML_MAX_MEMBERS 128
#define UML_MAX_PARAMS 32
#define UML_MAX_RELATIONS 256
#define UML_MAX_SEGMENTS 256
#define UML_MAX_POINTS 64
#define UML_MAX_ERRORS 128
#define UML_NAME_MAX 64
#define UML_TEXT_MAX 128

typedef enum {
    REL_NONE = 0,
    REL_HERENCIA,
    REL_DEPENDENCIA,
    REL_COMPOSICION,
    REL_AGREGACION
} RelationType;

typedef enum {
    VIS_PUBLICO = 0,
    VIS_PRIVADO,
    VIS_PROTEGIDO
} Visibility;

typedef struct {
    int line;
    int column;
    char message[256];
} UmlError;

typedef struct {
    UmlError items[UML_MAX_ERRORS];
    size_t count;
} ErrorList;

typedef struct {
    char name[UML_NAME_MAX];
    int x;
    int y;
    int line;
    int column;
} PositionDecl;

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    char from[UML_NAME_MAX];
    char to[UML_NAME_MAX];
    RelationType relation;
    Point points[UML_MAX_POINTS];
    size_t point_count;
    int line;
    int column;
} SegmentDecl;

typedef struct {
    Visibility visibility;
    char type[UML_NAME_MAX];
    char name[UML_NAME_MAX];
    int line;
    int column;
} AttributeDecl;

typedef struct {
    char type[UML_NAME_MAX];
    char name[UML_NAME_MAX];
} ParamDecl;

typedef struct {
    Visibility visibility;
    char return_type[UML_NAME_MAX];
    char name[UML_NAME_MAX];
    ParamDecl params[UML_MAX_PARAMS];
    size_t param_count;
    int line;
    int column;
} MethodDecl;

typedef struct {
    char name[UML_NAME_MAX];
    bool is_abstract;
    RelationType relation;
    char relation_target[UML_NAME_MAX];
    AttributeDecl attributes[UML_MAX_MEMBERS];
    size_t attribute_count;
    MethodDecl methods[UML_MAX_MEMBERS];
    size_t method_count;
    int line;
    int column;
} ClassDecl;

typedef struct {
    char from[UML_NAME_MAX];
    char to[UML_NAME_MAX];
    RelationType type;
    Point points[UML_MAX_POINTS];
    size_t point_count;
} RelationDecl;

typedef struct {
    char name[UML_NAME_MAX];
    PositionDecl positions[UML_MAX_CLASSES];
    size_t position_count;
    SegmentDecl segments[UML_MAX_SEGMENTS];
    size_t segment_count;
    ClassDecl classes[UML_MAX_CLASSES];
    size_t class_count;
    RelationDecl relations[UML_MAX_RELATIONS];
    size_t relation_count;
} Diagram;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int name_height;
    int attrs_height;
    int methods_height;
} ClassBox;

typedef struct {
    ClassBox boxes[UML_MAX_CLASSES];
    int width;
    int height;
} Layout;

void error_list_init(ErrorList *errors);
void error_add(ErrorList *errors, int line, int column, const char *fmt, ...);
void error_print_all(const ErrorList *errors);

void diagram_init(Diagram *diagram);
const ClassDecl *diagram_find_class(const Diagram *diagram, const char *name);
const PositionDecl *diagram_find_position(const Diagram *diagram, const char *name);
const char *relation_name(RelationType type);
const char *visibility_symbol(Visibility visibility);

bool parse_diagram(const char *source, Diagram *diagram, ErrorList *errors);
bool analyze_semantics(Diagram *diagram, ErrorList *errors);
void compute_layout(const Diagram *diagram, Layout *layout);
size_t compute_relation_points(const Diagram *diagram, const Layout *layout, const RelationDecl *relation, Point *points, size_t max_points);
bool render_dot(const Diagram *diagram, const Layout *layout, const char *path, ErrorList *errors);
bool render_svg(const Diagram *diagram, const Layout *layout, const char *path, ErrorList *errors);
bool render_ascii(const Diagram *diagram, const Layout *layout);
bool render_raylib_view(const Diagram *diagram, const Layout *layout, const char *png_path, ErrorList *errors);

#endif
