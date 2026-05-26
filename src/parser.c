#include "uml.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    TOK_EOF = 0,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_DIAGRAMA,
    TOK_POSICION,
    TOK_SEGMENTO,
    TOK_ABSTRACTA,
    TOK_CLASE,
    TOK_ATRIBUTOS,
    TOK_METODOS,
    TOK_PUBLICO,
    TOK_PRIVADO,
    TOK_PROTEGIDO,
    TOK_HERENCIA,
    TOK_DEPENDENCIA,
    TOK_COMPOSICION,
    TOK_AGREGACION,
    TOK_X,
    TOK_Y,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_SEMICOLON,
    TOK_COLON,
    TOK_DOT,
    TOK_EQUALS
} TokenType;

typedef struct {
    TokenType type;
    char lexeme[UML_TEXT_MAX];
    int number;
    int line;
    int column;
} Token;

typedef struct {
    const char *source;
    size_t pos;
    int line;
    int column;
    Token current;
    ErrorList *errors;
} Parser;

static void copy_text(char *dest, size_t dest_size, const char *start, size_t len) {
    if (len >= dest_size) {
        len = dest_size - 1;
    }
    memcpy(dest, start, len);
    dest[len] = '\0';
}

static int ci_equal(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return 0;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

static TokenType keyword_type(const char *text) {
    if (ci_equal(text, "Diagrama")) return TOK_DIAGRAMA;
    if (ci_equal(text, "posicion")) return TOK_POSICION;
    if (ci_equal(text, "segmento")) return TOK_SEGMENTO;
    if (ci_equal(text, "abstracta")) return TOK_ABSTRACTA;
    if (ci_equal(text, "clase")) return TOK_CLASE;
    if (ci_equal(text, "atributos")) return TOK_ATRIBUTOS;
    if (ci_equal(text, "metodos")) return TOK_METODOS;
    if (ci_equal(text, "publico")) return TOK_PUBLICO;
    if (ci_equal(text, "privado")) return TOK_PRIVADO;
    if (ci_equal(text, "protegido")) return TOK_PROTEGIDO;
    if (ci_equal(text, "herencia") || ci_equal(text, "hereda")) return TOK_HERENCIA;
    if (ci_equal(text, "dependencia")) return TOK_DEPENDENCIA;
    if (ci_equal(text, "composicion")) return TOK_COMPOSICION;
    if (ci_equal(text, "agregacion")) return TOK_AGREGACION;
    if (ci_equal(text, "X")) return TOK_X;
    if (ci_equal(text, "Y")) return TOK_Y;
    return TOK_IDENTIFIER;
}

static char peek(Parser *parser) {
    return parser->source[parser->pos];
}

static char advance_char(Parser *parser) {
    char c = parser->source[parser->pos++];
    if (c == '\n') {
        parser->line++;
        parser->column = 1;
    } else {
        parser->column++;
    }
    return c;
}

static void skip_ws_and_comments(Parser *parser) {
    for (;;) {
        while (isspace((unsigned char)peek(parser))) {
            advance_char(parser);
        }
        if (peek(parser) == '/' && parser->source[parser->pos + 1] == '/') {
            while (peek(parser) && peek(parser) != '\n') {
                advance_char(parser);
            }
            continue;
        }
        break;
    }
}

static Token next_token(Parser *parser) {
    skip_ws_and_comments(parser);

    Token token;
    memset(&token, 0, sizeof(token));
    token.line = parser->line;
    token.column = parser->column;

    char c = peek(parser);
    if (c == '\0') {
        token.type = TOK_EOF;
        return token;
    }

    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = parser->pos;
        while (isalnum((unsigned char)peek(parser)) || peek(parser) == '_') {
            advance_char(parser);
        }
        copy_text(token.lexeme, sizeof(token.lexeme), parser->source + start, parser->pos - start);
        token.type = keyword_type(token.lexeme);
        return token;
    }

    if (isdigit((unsigned char)c)) {
        size_t start = parser->pos;
        while (isdigit((unsigned char)peek(parser))) {
            advance_char(parser);
        }
        copy_text(token.lexeme, sizeof(token.lexeme), parser->source + start, parser->pos - start);
        token.number = atoi(token.lexeme);
        token.type = TOK_NUMBER;
        return token;
    }

    advance_char(parser);
    switch (c) {
        case '{': token.type = TOK_LBRACE; break;
        case '}': token.type = TOK_RBRACE; break;
        case '(': token.type = TOK_LPAREN; break;
        case ')': token.type = TOK_RPAREN; break;
        case ',': token.type = TOK_COMMA; break;
        case ';': token.type = TOK_SEMICOLON; break;
        case ':': token.type = TOK_COLON; break;
        case '.': token.type = TOK_DOT; break;
        case '=': token.type = TOK_EQUALS; break;
        default:
            token.type = TOK_EOF;
            error_add(parser->errors, token.line, token.column, "caracter inesperado '%c'", c);
            break;
    }
    return token;
}

static void advance(Parser *parser) {
    parser->current = next_token(parser);
}

static bool match(Parser *parser, TokenType type) {
    if (parser->current.type != type) {
        return false;
    }
    advance(parser);
    return true;
}

static bool expect(Parser *parser, TokenType type, const char *message) {
    if (parser->current.type == type) {
        advance(parser);
        return true;
    }
    error_add(parser->errors, parser->current.line, parser->current.column, "%s", message);
    return false;
}

static bool expect_identifier(Parser *parser, char *out, size_t out_size) {
    if (parser->current.type != TOK_IDENTIFIER) {
        error_add(parser->errors, parser->current.line, parser->current.column, "se esperaba un identificador");
        return false;
    }
    snprintf(out, out_size, "%s", parser->current.lexeme);
    advance(parser);
    return true;
}

static bool expect_number(Parser *parser, int *out) {
    if (parser->current.type != TOK_NUMBER) {
        error_add(parser->errors, parser->current.line, parser->current.column, "se esperaba un numero entero positivo");
        return false;
    }
    *out = parser->current.number;
    advance(parser);
    return true;
}

static RelationType token_relation(TokenType type) {
    switch (type) {
        case TOK_HERENCIA:
            return REL_HERENCIA;
        case TOK_DEPENDENCIA:
            return REL_DEPENDENCIA;
        case TOK_COMPOSICION:
            return REL_COMPOSICION;
        case TOK_AGREGACION:
            return REL_AGREGACION;
        default:
            return REL_NONE;
    }
}

static bool parse_header(Parser *parser, Diagram *diagram) {
    if (!expect(parser, TOK_DIAGRAMA, "se esperaba 'Diagrama'")) return false;
    if (!expect_identifier(parser, diagram->name, sizeof(diagram->name))) return false;
    return expect(parser, TOK_DOT, "se esperaba '.' despues del nombre del diagrama");
}

static bool parse_position(Parser *parser, Diagram *diagram) {
    if (diagram->position_count >= UML_MAX_CLASSES) {
        error_add(parser->errors, parser->current.line, parser->current.column, "demasiadas posiciones declaradas");
        return false;
    }

    PositionDecl *position = &diagram->positions[diagram->position_count++];
    memset(position, 0, sizeof(*position));
    position->line = parser->current.line;
    position->column = parser->current.column;

    expect(parser, TOK_POSICION, "se esperaba 'posicion'");
    if (!expect_identifier(parser, position->name, sizeof(position->name))) return false;
    if (!expect(parser, TOK_LBRACE, "se esperaba '{' en posicion")) return false;
    if (!expect(parser, TOK_X, "se esperaba 'X'")) return false;
    if (!expect(parser, TOK_EQUALS, "se esperaba '=' despues de X")) return false;
    if (!expect_number(parser, &position->x)) return false;
    if (!expect(parser, TOK_SEMICOLON, "se esperaba ';' despues de X")) return false;
    if (!expect(parser, TOK_Y, "se esperaba 'Y'")) return false;
    if (!expect(parser, TOK_EQUALS, "se esperaba '=' despues de Y")) return false;
    if (!expect_number(parser, &position->y)) return false;
    if (!expect(parser, TOK_SEMICOLON, "se esperaba ';' despues de Y")) return false;
    return expect(parser, TOK_RBRACE, "se esperaba '}' al cerrar posicion");
}

static bool parse_segment(Parser *parser, Diagram *diagram) {
    if (diagram->segment_count >= UML_MAX_SEGMENTS) {
        error_add(parser->errors, parser->current.line, parser->current.column, "demasiados segmentos declarados");
        return false;
    }

    SegmentDecl *segment = &diagram->segments[diagram->segment_count++];
    memset(segment, 0, sizeof(*segment));
    segment->line = parser->current.line;
    segment->column = parser->current.column;

    expect(parser, TOK_SEGMENTO, "se esperaba 'segmento'");
    if (!expect(parser, TOK_LPAREN, "se esperaba '(' en segmento")) return false;
    if (!expect_identifier(parser, segment->from, sizeof(segment->from))) return false;
    if (!expect(parser, TOK_COMMA, "se esperaba ',' despues de la clase origen")) return false;
    if (!expect_identifier(parser, segment->to, sizeof(segment->to))) return false;

    while (match(parser, TOK_COMMA)) {
        if (token_relation(parser->current.type) != REL_NONE) {
            segment->relation = token_relation(parser->current.type);
            advance(parser);
            break;
        }

        if (segment->point_count >= UML_MAX_POINTS) {
            error_add(parser->errors, parser->current.line, parser->current.column, "demasiados puntos en segmento");
            return false;
        }

        Point *point = &segment->points[segment->point_count++];
        if (!expect_number(parser, &point->x)) return false;
        if (!expect(parser, TOK_COMMA, "se esperaba ',' entre coordenadas del segmento")) return false;
        if (!expect_number(parser, &point->y)) return false;
    }

    if (segment->relation == REL_NONE) {
        error_add(parser->errors, segment->line, segment->column, "el segmento debe terminar con un tipo de relacion");
        return false;
    }

    if (!expect(parser, TOK_RPAREN, "se esperaba ')' al cerrar segmento")) return false;
    return expect(parser, TOK_SEMICOLON, "se esperaba ';' despues de segmento");
}

static bool parse_visibility(Parser *parser, Visibility *visibility) {
    if (parser->current.type == TOK_PUBLICO) {
        *visibility = VIS_PUBLICO;
    } else if (parser->current.type == TOK_PRIVADO) {
        *visibility = VIS_PRIVADO;
    } else if (parser->current.type == TOK_PROTEGIDO) {
        *visibility = VIS_PROTEGIDO;
    } else {
        error_add(parser->errors, parser->current.line, parser->current.column, "se esperaba visibilidad: publico, privado o protegido");
        return false;
    }
    advance(parser);
    return true;
}

static bool parse_attribute(Parser *parser, ClassDecl *klass) {
    if (klass->attribute_count >= UML_MAX_MEMBERS) {
        error_add(parser->errors, parser->current.line, parser->current.column, "demasiados atributos en la clase");
        return false;
    }

    Visibility visibility;
    if (!parse_visibility(parser, &visibility)) return false;

    char type[UML_NAME_MAX];
    if (!expect_identifier(parser, type, sizeof(type))) return false;

    for (;;) {
        if (klass->attribute_count >= UML_MAX_MEMBERS) {
            error_add(parser->errors, parser->current.line, parser->current.column, "demasiados atributos en la clase");
            return false;
        }
        AttributeDecl *attribute = &klass->attributes[klass->attribute_count++];
        memset(attribute, 0, sizeof(*attribute));
        attribute->visibility = visibility;
        snprintf(attribute->type, sizeof(attribute->type), "%s", type);
        attribute->line = parser->current.line;
        attribute->column = parser->current.column;
        if (!expect_identifier(parser, attribute->name, sizeof(attribute->name))) return false;
        if (!match(parser, TOK_COMMA)) {
            break;
        }
    }

    return expect(parser, TOK_SEMICOLON, "se esperaba ';' despues de atributo");
}

static bool parse_method(Parser *parser, ClassDecl *klass) {
    if (klass->method_count >= UML_MAX_MEMBERS) {
        error_add(parser->errors, parser->current.line, parser->current.column, "demasiados metodos en la clase");
        return false;
    }

    MethodDecl *method = &klass->methods[klass->method_count++];
    memset(method, 0, sizeof(*method));
    method->line = parser->current.line;
    method->column = parser->current.column;

    if (!parse_visibility(parser, &method->visibility)) return false;
    if (!expect_identifier(parser, method->return_type, sizeof(method->return_type))) return false;
    if (!expect_identifier(parser, method->name, sizeof(method->name))) return false;
    if (!expect(parser, TOK_LPAREN, "se esperaba '(' en metodo")) return false;

    if (parser->current.type != TOK_RPAREN) {
        for (;;) {
            if (method->param_count >= UML_MAX_PARAMS) {
                error_add(parser->errors, parser->current.line, parser->current.column, "demasiados parametros en metodo");
                return false;
            }
            ParamDecl *param = &method->params[method->param_count++];
            if (!expect_identifier(parser, param->type, sizeof(param->type))) return false;
            if (!expect_identifier(parser, param->name, sizeof(param->name))) return false;
            if (!match(parser, TOK_COMMA)) {
                break;
            }
        }
    }

    if (!expect(parser, TOK_RPAREN, "se esperaba ')' en metodo")) return false;
    return expect(parser, TOK_SEMICOLON, "se esperaba ';' despues de metodo");
}

static bool parse_class(Parser *parser, Diagram *diagram) {
    if (diagram->class_count >= UML_MAX_CLASSES) {
        error_add(parser->errors, parser->current.line, parser->current.column, "demasiadas clases declaradas");
        return false;
    }

    ClassDecl *klass = &diagram->classes[diagram->class_count++];
    memset(klass, 0, sizeof(*klass));
    klass->line = parser->current.line;
    klass->column = parser->current.column;

    klass->is_abstract = match(parser, TOK_ABSTRACTA);
    if (!expect(parser, TOK_CLASE, "se esperaba 'clase'")) return false;
    if (!expect_identifier(parser, klass->name, sizeof(klass->name))) return false;

    RelationType relation = token_relation(parser->current.type);
    if (relation != REL_NONE) {
        klass->relation = relation;
        advance(parser);
        if (!expect_identifier(parser, klass->relation_target, sizeof(klass->relation_target))) return false;
    }

    if (!expect(parser, TOK_LBRACE, "se esperaba '{' en clase")) return false;

    if (match(parser, TOK_ATRIBUTOS)) {
        if (!expect(parser, TOK_COLON, "se esperaba ':' despues de atributos")) return false;
        while (parser->current.type == TOK_PUBLICO || parser->current.type == TOK_PRIVADO || parser->current.type == TOK_PROTEGIDO) {
            if (!parse_attribute(parser, klass)) return false;
        }
    }

    if (match(parser, TOK_METODOS)) {
        if (!expect(parser, TOK_COLON, "se esperaba ':' despues de metodos")) return false;
        while (parser->current.type == TOK_PUBLICO || parser->current.type == TOK_PRIVADO || parser->current.type == TOK_PROTEGIDO) {
            if (!parse_method(parser, klass)) return false;
        }
    }

    return expect(parser, TOK_RBRACE, "se esperaba '}' al cerrar clase");
}

bool parse_diagram(const char *source, Diagram *diagram, ErrorList *errors) {
    Parser parser;
    memset(&parser, 0, sizeof(parser));
    parser.source = source;
    parser.line = 1;
    parser.column = 1;
    parser.errors = errors;

    diagram_init(diagram);
    advance(&parser);

    if (!parse_header(&parser, diagram)) {
        return false;
    }

    while (parser.current.type == TOK_POSICION) {
        if (!parse_position(&parser, diagram)) return false;
    }

    while (parser.current.type == TOK_SEGMENTO) {
        if (!parse_segment(&parser, diagram)) return false;
    }

    while (parser.current.type == TOK_ABSTRACTA || parser.current.type == TOK_CLASE) {
        if (!parse_class(&parser, diagram)) return false;
    }

    if (parser.current.type != TOK_EOF) {
        error_add(errors, parser.current.line, parser.current.column, "entrada inesperada al final del programa");
        return false;
    }

    return errors->count == 0;
}
