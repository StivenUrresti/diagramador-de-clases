#include "uml.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    const char *input_path;
    const char *dot_path;
    const char *svg_path;
    const char *png_path;
    bool view;
    bool ascii;
    bool open_svg;
} CliOptions;

static void print_usage(const char *program) {
    fprintf(stderr, "Uso: %s archivo.uml [--dot salida.dot] [--svg salida.svg] [--png salida.png] [--view] [--ascii] [--open]\n", program);
    fprintf(stderr, "Sin --dot ni --svg genera build/<nombre>.dot y build/<nombre>.svg\n");
}

static const char *path_basename(const char *path) {
    const char *base = path;
    const char *slash = strrchr(path, '/');
    const char *backslash = strrchr(path, '\\');

    if (slash != NULL && slash + 1 > base) {
        base = slash + 1;
    }
    if (backslash != NULL && backslash + 1 > base) {
        base = backslash + 1;
    }
    return base;
}

static bool derive_stem(const char *input_path, char *stem, size_t stem_size) {
    const char *base = path_basename(input_path);
    const char *dot = strrchr(base, '.');
    size_t len = dot != NULL ? (size_t)(dot - base) : strlen(base);

    if (len == 0 || len >= stem_size) {
        return false;
    }

    memcpy(stem, base, len);
    stem[len] = '\0';
    return true;
}

static bool ensure_build_dir(void) {
    struct stat info;
    if (stat("build", &info) == 0) {
        return S_ISDIR(info.st_mode);
    }
    return mkdir("build", 0755) == 0;
}

static bool apply_default_outputs(const char *input_path, char *dot_path, size_t dot_size, char *svg_path, size_t svg_size, CliOptions *options) {
    char stem[256];

    if (!derive_stem(input_path, stem, sizeof(stem))) {
        return false;
    }
    if (!ensure_build_dir()) {
        return false;
    }
    {
        int dot_written = snprintf(dot_path, dot_size, "build/%s.dot", stem);
        int svg_written = snprintf(svg_path, svg_size, "build/%s.svg", stem);
        if (dot_written < 0 || (size_t)dot_written >= dot_size || svg_written < 0 || (size_t)svg_written >= svg_size) {
            return false;
        }
    }

    if (options->dot_path == NULL && options->svg_path == NULL && options->png_path == NULL && !options->view && !options->ascii) {
        options->dot_path = dot_path;
        options->svg_path = svg_path;
    } else if (options->open_svg && options->svg_path == NULL) {
        options->svg_path = svg_path;
        if (options->dot_path == NULL) {
            options->dot_path = dot_path;
        }
    }

    return true;
}

static bool parse_args(int argc, char **argv, CliOptions *options) {
    memset(options, 0, sizeof(*options));

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--dot") == 0) {
            if (i + 1 >= argc) return false;
            options->dot_path = argv[++i];
        } else if (strcmp(argv[i], "--svg") == 0) {
            if (i + 1 >= argc) return false;
            options->svg_path = argv[++i];
        } else if (strcmp(argv[i], "--png") == 0) {
            if (i + 1 >= argc) return false;
            options->png_path = argv[++i];
        } else if (strcmp(argv[i], "--view") == 0) {
            options->view = true;
        } else if (strcmp(argv[i], "--ascii") == 0) {
            options->ascii = true;
        } else if (strcmp(argv[i], "--open") == 0) {
            options->open_svg = true;
        } else if (argv[i][0] == '-') {
            return false;
        } else if (options->input_path == NULL) {
            options->input_path = argv[i];
        } else {
            return false;
        }
    }

    if (options->input_path == NULL) {
        return false;
    }

    if (options->png_path != NULL) {
        options->view = true;
    }

    return true;
}

static bool svg_viewer_path(const char *svg_path, char *viewer_path, size_t size) {
    int written = snprintf(viewer_path, size, "%s.html", svg_path);
    return written >= 0 && (size_t)written < size;
}

static void remove_requested_outputs(const CliOptions *options) {
    if (options->dot_path != NULL) {
        remove(options->dot_path);
    }
    if (options->svg_path != NULL) {
        char viewer_path[1024];
        if (svg_viewer_path(options->svg_path, viewer_path, sizeof(viewer_path))) {
            remove(viewer_path);
        }
        remove(options->svg_path);
    }
    if (options->png_path != NULL) {
        remove(options->png_path);
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

static bool open_svg_file(const char *path, ErrorList *errors) {
    char viewer_path[1024];
    if (!svg_viewer_path(path, viewer_path, sizeof(viewer_path))) {
        error_add(errors, 0, 0, "ruta demasiado larga para crear visor HTML de '%s'", path);
        return false;
    }

    const char *file_name = strrchr(path, '/');
    file_name = file_name == NULL ? path : file_name + 1;

    FILE *viewer = fopen(viewer_path, "w");
    if (viewer == NULL) {
        error_add(errors, 0, 0, "no se pudo crear visor HTML '%s'", viewer_path);
        return false;
    }

    fprintf(viewer, "<!doctype html>\n<html lang=\"es\">\n<head>\n");
    fprintf(viewer, "<meta charset=\"utf-8\">\n<title>Mini PlantUML SVG</title>\n");
    fprintf(viewer, "<style>body{margin:0;background:#f7f7f7;}img{max-width:100vw;height:auto;display:block;margin:0 auto;}</style>\n");
    fprintf(viewer, "</head>\n<body>\n<img src=\"");
    html_text(viewer, file_name);
    fprintf(viewer, "\" alt=\"Diagrama UML generado\">\n</body>\n</html>\n");
    fclose(viewer);

    char command[1024];
    int written = snprintf(command, sizeof(command), "open \"%s\"", viewer_path);
    if (written < 0 || (size_t)written >= sizeof(command)) {
        error_add(errors, 0, 0, "ruta demasiado larga para abrir visor '%s'", viewer_path);
        return false;
    }

    int exit_code = system(command);
    if (exit_code != 0) {
        error_add(errors, 0, 0, "no se pudo abrir visor HTML '%s'", viewer_path);
        return false;
    }
    printf("Visor abierto: %s\n", viewer_path);
    return true;
}

static char *read_input_file(const char *path, ErrorList *errors) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        error_add(errors, 0, 0, "no se pudo abrir '%s'", path);
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        error_add(errors, 0, 0, "no se pudo leer '%s'", path);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        error_add(errors, 0, 0, "no se pudo obtener el tamano de '%s'", path);
        return NULL;
    }
    rewind(file);

    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        error_add(errors, 0, 0, "memoria insuficiente para leer '%s'", path);
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    buffer[read] = '\0';
    return buffer;
}

int main(int argc, char **argv) {
    CliOptions options;
    char default_dot_path[512];
    char default_svg_path[512];

    if (!parse_args(argc, argv, &options)) {
        print_usage(argv[0]);
        return 2;
    }

    if (!apply_default_outputs(options.input_path, default_dot_path, sizeof(default_dot_path), default_svg_path, sizeof(default_svg_path), &options)) {
        fprintf(stderr, "No se pudieron derivar rutas de salida para '%s'\n", options.input_path);
        print_usage(argv[0]);
        return 2;
    }

    ErrorList errors;
    error_list_init(&errors);

    char *source = read_input_file(options.input_path, &errors);
    if (source == NULL) {
        remove_requested_outputs(&options);
        error_print_all(&errors);
        return 1;
    }

    Diagram *diagram = calloc(1, sizeof(*diagram));
    Layout *layout = calloc(1, sizeof(*layout));
    if (diagram == NULL || layout == NULL) {
        free(source);
        free(diagram);
        free(layout);
        error_add(&errors, 0, 0, "memoria insuficiente para crear el diagrama");
        remove_requested_outputs(&options);
        error_print_all(&errors);
        return 1;
    }

    bool ok = parse_diagram(source, diagram, &errors);
    free(source);

    if (ok) {
        ok = analyze_semantics(diagram, &errors);
    }

    if (!ok) {
        remove_requested_outputs(&options);
        error_print_all(&errors);
        free(diagram);
        free(layout);
        return 1;
    }

    compute_layout(diagram, layout);
    plan_routes(diagram, layout);

    if (options.ascii) {
        render_ascii(diagram, layout);
    }

    if (options.dot_path != NULL && !render_dot(diagram, layout, options.dot_path, &errors)) {
        error_print_all(&errors);
        free(diagram);
        free(layout);
        return 1;
    }

    if (options.svg_path != NULL && !render_svg(diagram, layout, options.svg_path, &errors)) {
        error_print_all(&errors);
        free(diagram);
        free(layout);
        return 1;
    }

    if (options.view && !render_raylib_view(diagram, layout, options.png_path, &errors)) {
        error_print_all(&errors);
        if (options.png_path != NULL && options.svg_path == NULL) {
            free(diagram);
            free(layout);
            return 1;
        }
    }

    if (options.dot_path != NULL) {
        printf("DOT generado: %s\n", options.dot_path);
    }
    if (options.svg_path != NULL) {
        printf("SVG generado: %s\n", options.svg_path);
    }
    if (options.png_path != NULL) {
        printf("PNG solicitado: %s\n", options.png_path);
    }
    if (options.open_svg && options.svg_path != NULL && !open_svg_file(options.svg_path, &errors)) {
        error_print_all(&errors);
        free(diagram);
        free(layout);
        return 1;
    }

    free(diagram);
    free(layout);
    return 0;
}
