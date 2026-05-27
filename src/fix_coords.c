#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POSITIONS 128
#define NAME_MAX 64
#define LINE_MAX 512

enum {
    COL_BASE = 80,
    COL_STEP = 250,
    ROW_BASE = 50,
    ROW_STEP = 230
};

typedef struct {
    char name[NAME_MAX];
    int x;
    int y;
    int new_x;
    int new_y;
    bool changed;
} PositionEntry;

typedef struct {
    PositionEntry items[MAX_POSITIONS];
    size_t count;
} PositionList;

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }

    rewind(file);
    char *buffer = malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }

    size_t read = fread(buffer, 1, (size_t)size, file);
    fclose(file);
    buffer[read] = '\0';
    return buffer;
}

static bool write_file(const char *path, const char *content) {
    FILE *file = fopen(path, "w");
    if (file == NULL) {
        return false;
    }
    fputs(content, file);
    fclose(file);
    return true;
}

static bool load_positions(const char *path, PositionList *positions) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        fprintf(stderr, "No se pudo abrir '%s'\n", path);
        return false;
    }

    positions->count = 0;
    char line[LINE_MAX];
    bool in_block = false;
    bool have_x = false;

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strncmp(line, "posicion ", 9) == 0) {
            if (positions->count >= MAX_POSITIONS) {
                fprintf(stderr, "Demasiadas posiciones en '%s'\n", path);
                fclose(file);
                return false;
            }
            PositionEntry *entry = &positions->items[positions->count++];
            memset(entry, 0, sizeof(*entry));
            if (sscanf(line + 9, "%63s", entry->name) != 1) {
                fclose(file);
                return false;
            }
            size_t len = strlen(entry->name);
            if (len > 0 && entry->name[len - 1] == '{') {
                entry->name[len - 1] = '\0';
            }
            in_block = true;
            have_x = false;
            continue;
        }

        if (!in_block || positions->count == 0) {
            continue;
        }

        PositionEntry *current = &positions->items[positions->count - 1];
        if (sscanf(line, " X=%d;", &current->x) == 1 || sscanf(line, "X=%d;", &current->x) == 1) {
            have_x = true;
        } else if (have_x && (sscanf(line, " Y=%d;", &current->y) == 1 || sscanf(line, "Y=%d;", &current->y) == 1)) {
            in_block = false;
            have_x = false;
        } else if (strchr(line, '}') != NULL) {
            in_block = false;
            have_x = false;
        }
    }

    fclose(file);
    return true;
}

static int compare_int(const void *a, const void *b) {
    return *(const int *)a - *(const int *)b;
}

static int index_in_sorted(const int *values, size_t count, int value) {
    for (size_t i = 0; i < count; i++) {
        if (values[i] == value) {
            return (int)i;
        }
    }
    return -1;
}

static int canonical_x(int column_index) {
    return COL_BASE + column_index * COL_STEP;
}

static int canonical_y(int row_index) {
    return ROW_BASE + row_index * ROW_STEP;
}

static bool same_pair(int x1, int y1, int x2, int y2) {
    return x1 == x2 && y1 == y2;
}

static void report_duplicates(const PositionList *positions) {
    for (size_t i = 0; i < positions->count; i++) {
        for (size_t j = i + 1; j < positions->count; j++) {
            const PositionEntry *a = &positions->items[i];
            const PositionEntry *b = &positions->items[j];
            if (same_pair(a->x, a->y, b->x, b->y)) {
                printf("Duplicado: '%s' y '%s' en (%d,%d)\n", a->name, b->name, a->x, a->y);
            }
        }
    }
}

static void resolve_and_normalize(PositionList *positions) {
    int xs[MAX_POSITIONS];
    int ys[MAX_POSITIONS];
    size_t x_count = 0;
    size_t y_count = 0;

    for (size_t i = 0; i < positions->count; i++) {
        PositionEntry *entry = &positions->items[i];
        entry->new_x = entry->x;
        entry->new_y = entry->y;

        if (index_in_sorted(xs, x_count, entry->x) < 0) {
            xs[x_count++] = entry->x;
        }
        if (index_in_sorted(ys, y_count, entry->y) < 0) {
            ys[y_count++] = entry->y;
        }
    }

    qsort(xs, x_count, sizeof(int), compare_int);
    qsort(ys, y_count, sizeof(int), compare_int);

    for (size_t i = 0; i < positions->count; i++) {
        for (size_t j = i + 1; j < positions->count; j++) {
            PositionEntry *b = &positions->items[j];
            const PositionEntry *a = &positions->items[i];
            if (!same_pair(a->x, a->y, b->x, b->y)) {
                continue;
            }

            int y_index = index_in_sorted(ys, y_count, b->y);
            if (y_index >= 0 && (size_t)(y_index + 1) < y_count) {
                b->new_y = ys[y_index + 1];
            } else {
                int next_y = (y_count > 0 ? ys[y_count - 1] : b->y) + ROW_STEP;
                if (y_count < MAX_POSITIONS) {
                    ys[y_count++] = next_y;
                    qsort(ys, y_count, sizeof(int), compare_int);
                }
                b->new_y = next_y;
            }
        }
    }

    for (size_t i = 0; i < positions->count; i++) {
        PositionEntry *entry = &positions->items[i];
        int x_index = index_in_sorted(xs, x_count, entry->x);
        int y_index = index_in_sorted(ys, y_count, entry->new_y);
        if (x_index < 0) {
            x_index = 0;
        }
        if (y_index < 0) {
            y_index = 0;
        }

        int fixed_x = canonical_x(x_index);
        int fixed_y = canonical_y(y_index);
        entry->changed = entry->x != fixed_x || entry->y != fixed_y || entry->new_y != entry->y;
        entry->new_x = fixed_x;
        entry->new_y = fixed_y;
    }
}

static const PositionEntry *find_position(const PositionList *positions, const char *name) {
    for (size_t i = 0; i < positions->count; i++) {
        if (strcmp(positions->items[i].name, name) == 0) {
            return &positions->items[i];
        }
    }
    return NULL;
}

static char *rewrite_file(const char *input, const PositionList *positions) {
    size_t capacity = strlen(input) + 1024;
    char *output = malloc(capacity);
    if (output == NULL) {
        return NULL;
    }

    size_t used = 0;
    output[0] = '\0';

    char line[LINE_MAX];
    char current_name[NAME_MAX];
    bool in_block = false;
    bool have_x = false;
    const char *cursor = input;

    while (true) {
        size_t line_len = 0;
        while (cursor[line_len] != '\0' && cursor[line_len] != '\n') {
            line_len++;
        }
        if (line_len == 0 && *cursor == '\0') {
            break;
        }

        if (line_len >= sizeof(line)) {
            line_len = sizeof(line) - 1;
        }
        memcpy(line, cursor, line_len);
        line[line_len] = '\0';

        char rewritten[LINE_MAX];
        const char *out_line = line;

        if (strncmp(line, "posicion ", 9) == 0) {
            if (sscanf(line + 9, "%63s", current_name) == 1) {
                size_t len = strlen(current_name);
                if (len > 0 && current_name[len - 1] == '{') {
                    current_name[len - 1] = '\0';
                }
            }
            in_block = true;
            have_x = false;
        } else if (in_block) {
            const PositionEntry *entry = find_position(positions, current_name);
            int value = 0;
            if (entry != NULL && (sscanf(line, " X=%d;", &value) == 1 || sscanf(line, "X=%d;", &value) == 1)) {
                snprintf(rewritten, sizeof(rewritten), "X=%d;", entry->new_x);
                out_line = rewritten;
                have_x = true;
            } else if (entry != NULL && have_x && (sscanf(line, " Y=%d;", &value) == 1 || sscanf(line, "Y=%d;", &value) == 1)) {
                snprintf(rewritten, sizeof(rewritten), "Y=%d;", entry->new_y);
                out_line = rewritten;
                in_block = false;
                have_x = false;
            } else if (strchr(line, '}') != NULL) {
                in_block = false;
                have_x = false;
            }
        }

        size_t out_len = strlen(out_line);
        if (cursor[line_len] == '\n') {
            if (used + out_len + 2 > capacity) {
                capacity = (used + out_len + 2) * 2;
                char *grown = realloc(output, capacity);
                if (grown == NULL) {
                    free(output);
                    return NULL;
                }
                output = grown;
            }
            memcpy(output + used, out_line, out_len);
            used += out_len;
            output[used++] = '\n';
            output[used] = '\0';
            cursor += line_len + 1;
        } else {
            if (used + out_len + 1 > capacity) {
                capacity = (used + out_len + 1) * 2;
                char *grown = realloc(output, capacity);
                if (grown == NULL) {
                    free(output);
                    return NULL;
                }
                output = grown;
            }
            memcpy(output + used, out_line, out_len);
            used += out_len;
            output[used] = '\0';
            break;
        }
    }

    return output;
}

static void print_summary(const PositionList *positions) {
    size_t changes = 0;
    for (size_t i = 0; i < positions->count; i++) {
        const PositionEntry *entry = &positions->items[i];
        if (entry->changed) {
            printf("Corregido: '%s' (%d,%d) -> (%d,%d)\n",
                   entry->name, entry->x, entry->y, entry->new_x, entry->new_y);
            changes++;
        } else {
            printf("OK: '%s' (%d,%d)\n", entry->name, entry->x, entry->y);
        }
    }

    if (changes == 0) {
        printf("No hubo cambios. Las coordenadas ya estan normalizadas.\n");
    }
}

static void print_usage(const char *program) {
    fprintf(stderr, "Uso: %s archivo.uml [--check]\n", program);
    fprintf(stderr, "Normaliza X/Y del .uml a la rejilla: X=80+250*n, Y=50+230*n\n");
    fprintf(stderr, "  --check   solo reporta, no modifica el archivo\n");
}

int main(int argc, char **argv) {
    bool check_only = false;
    const char *input_path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--check") == 0) {
            check_only = true;
        } else if (argv[i][0] == '-') {
            print_usage(argv[0]);
            return 2;
        } else if (input_path == NULL) {
            input_path = argv[i];
        } else {
            print_usage(argv[0]);
            return 2;
        }
    }

    if (input_path == NULL) {
        print_usage(argv[0]);
        return 2;
    }

    PositionList positions;
    if (!load_positions(input_path, &positions)) {
        return 1;
    }

    if (positions.count == 0) {
        fprintf(stderr, "No se encontraron bloques posicion en '%s'\n", input_path);
        return 1;
    }

    printf("Rejilla objetivo: columnas X=%d+%d*n, filas Y=%d+%d*n\n\n",
           COL_BASE, COL_STEP, ROW_BASE, ROW_STEP);

    report_duplicates(&positions);
    resolve_and_normalize(&positions);
    print_summary(&positions);

    if (check_only) {
        return 0;
    }

    bool needs_write = false;
    for (size_t i = 0; i < positions.count; i++) {
        if (positions.items[i].changed) {
            needs_write = true;
            break;
        }
    }

    if (!needs_write) {
        return 0;
    }

    char *input = read_file(input_path);
    if (input == NULL) {
        fprintf(stderr, "No se pudo leer '%s'\n", input_path);
        return 1;
    }

    char backup_path[1024];
    if (snprintf(backup_path, sizeof(backup_path), "%s.bak", input_path) >= (int)sizeof(backup_path)) {
        fprintf(stderr, "Ruta de respaldo demasiado larga\n");
        free(input);
        return 1;
    }

    if (!write_file(backup_path, input)) {
        fprintf(stderr, "Advertencia: no se pudo crear respaldo '%s'\n", backup_path);
    }

    char *output = rewrite_file(input, &positions);
    free(input);
    if (output == NULL) {
        fprintf(stderr, "No se pudo reescribir '%s'\n", input_path);
        return 1;
    }

    if (!write_file(input_path, output)) {
        fprintf(stderr, "No se pudo escribir '%s'\n", input_path);
        free(output);
        return 1;
    }

    free(output);
    printf("\nArchivo actualizado: %s\n", input_path);
    printf("Respaldo creado: %s\n", backup_path);
    return 0;
}
