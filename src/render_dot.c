#include "uml.h"

#include <stdio.h>
#include <string.h>

static void quoted_text(FILE *out, const char *text) {
    for (const char *p = text; *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', out);
        }
        fputc(*p, out);
    }
}

static void html_text(FILE *out, const char *text) {
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

static void html_class_label(FILE *out, const ClassDecl *klass) {
    fprintf(out, "<<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n");
    fprintf(out, "      <TR><TD ALIGN=\"CENTER\">");
    if (klass->is_abstract) {
        fprintf(out, "<I>");
    }
    html_text(out, klass->name);
    if (klass->is_abstract) {
        fprintf(out, "</I>");
    }
    fprintf(out, "</TD></TR>\n");

    fprintf(out, "      <TR><TD ALIGN=\"LEFT\">");
    if (klass->attribute_count == 0) {
        fprintf(out, "&#160;");
    } else {
        for (size_t i = 0; i < klass->attribute_count; i++) {
            if (i > 0) {
                fprintf(out, "<BR ALIGN=\"LEFT\"/>");
            }
            fprintf(out, "%s ", visibility_symbol(klass->attributes[i].visibility));
            html_text(out, klass->attributes[i].name);
            fprintf(out, ": ");
            html_text(out, klass->attributes[i].type);
        }
    }
    fprintf(out, "</TD></TR>\n");

    fprintf(out, "      <TR><TD ALIGN=\"LEFT\">");
    if (klass->method_count == 0) {
        fprintf(out, "&#160;");
    } else {
        for (size_t i = 0; i < klass->method_count; i++) {
            const MethodDecl *method = &klass->methods[i];
            if (i > 0) {
                fprintf(out, "<BR ALIGN=\"LEFT\"/>");
            }
            fprintf(out, "%s ", visibility_symbol(method->visibility));
            html_text(out, method->name);
            fprintf(out, "(");
            if (method->param_count > 0 && method->name[0] != '\0') {
                size_t approx_len = 3 + strlen(method->name) + strlen(method->return_type);
                for (size_t p = 0; p < method->param_count; p++) {
                    approx_len += strlen(method->params[p].name) + strlen(method->params[p].type) + 4;
                }
                if (approx_len > 34) {
                    fprintf(out, "<BR ALIGN=\"LEFT\"/>&#160;&#160;");
                }
            }
            for (size_t p = 0; p < method->param_count; p++) {
                if (p > 0) {
                    fprintf(out, ", ");
                }
                html_text(out, method->params[p].name);
                fprintf(out, ": ");
                html_text(out, method->params[p].type);
            }
            fprintf(out, "): ");
            html_text(out, method->return_type);
        }
    }
    fprintf(out, "</TD></TR>\n");
    fprintf(out, "    </TABLE>>");
}

static const char *dot_arrowhead(RelationType type) {
    switch (type) {
        case REL_HERENCIA:
            return "empty";
        case REL_DEPENDENCIA:
            return "open";
        case REL_COMPOSICION:
            return "diamond";
        case REL_AGREGACION:
            return "odiamond";
        case REL_NONE:
        default:
            return "none";
    }
}

static const char *dot_style(RelationType type) {
    return type == REL_DEPENDENCIA ? "dashed" : "solid";
}

bool render_dot(const Diagram *diagram, const Layout *layout, const char *path, ErrorList *errors) {
    FILE *out = fopen(path, "w");
    if (out == NULL) {
        error_add(errors, 0, 0, "no se pudo crear DOT '%s'", path);
        return false;
    }

    fprintf(out, "digraph \"");
    quoted_text(out, diagram->name);
    fprintf(out, "\" {\n");
    fprintf(out, "  graph [rankdir=TB, splines=ortho, overlap=false, nodesep=0.6, ranksep=0.7];\n");
    fprintf(out, "  node [shape=plain, fontname=\"monospace\", fontsize=10];\n");
    fprintf(out, "  edge [fontname=\"monospace\", fontsize=10];\n\n");

    for (size_t i = 0; i < diagram->class_count; i++) {
        const ClassDecl *klass = &diagram->classes[i];
        const ClassBox *box = &layout->boxes[i];
        fprintf(out, "  \"");
        quoted_text(out, klass->name);
        fprintf(out, "\" [pos=\"%d,%d!\", pin=true, label=", box->x, -box->y);
        html_class_label(out, klass);
        fprintf(out, "];\n");
    }

    fprintf(out, "\n");
    for (size_t i = 0; i < diagram->relation_count; i++) {
        const RelationDecl *relation = &diagram->relations[i];
        fprintf(out, "  \"");
        quoted_text(out, relation->from);
        fprintf(out, "\" -> \"");
        quoted_text(out, relation->to);
        fprintf(out, "\" [label=\"%s\", style=%s, arrowhead=%s",
                relation_name(relation->type),
                dot_style(relation->type),
                dot_arrowhead(relation->type));
        if (relation->point_count >= 2) {
            fprintf(out, ", route=\"");
            for (size_t p = 0; p < relation->point_count; p++) {
                fprintf(out, "%s%d,%d", p == 0 ? "" : " ", relation->points[p].x, relation->points[p].y);
            }
            fprintf(out, "\"");
        }
        fprintf(out, "];\n");
    }

    fprintf(out, "}\n");
    fclose(out);
    return true;
}
