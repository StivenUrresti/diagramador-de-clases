CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?= -lm
BUILD_DIR := build
TARGET := $(BUILD_DIR)/uml_engine
SOURCES := src/main.c src/parser.c src/semantic.c src/layout.c src/route.c src/render_dot.c src/render_svg.c src/render_raylib.c src/util.c
OBJECTS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run preview diagram test raylib

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: src/%.c include/uml.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

raylib:
	$(MAKE) clean
	$(MAKE) CFLAGS="$(CFLAGS) -DUSE_RAYLIB $$(pkg-config --cflags raylib 2>/dev/null)" LDFLAGS="$(LDFLAGS) $$(pkg-config --libs raylib 2>/dev/null || echo -lraylib)" $(TARGET)

run: $(TARGET)
	$(TARGET) examples/juego.uml --open

preview: $(TARGET)
	$(TARGET) examples/juego.uml --open

diagram: $(TARGET)
	@test -n "$(UML)" || (echo "Uso: make diagram UML=examples/cuatro.uml" && exit 1)
	$(TARGET) $(UML) --open

test: $(TARGET)
	@$(TARGET) examples/juego.uml --dot build/juego.dot --svg build/juego.svg >/dev/null
	@echo "OK: ejemplo valido genera DOT y SVG"
	@rm -f build/duplicados.dot build/duplicados.svg; \
	$(TARGET) tests/duplicados.uml --dot build/duplicados.dot --svg build/duplicados.svg >/dev/null 2>&1; \
	if [ $$? -eq 0 ]; then \
		echo "ERROR: tests/duplicados.uml debia fallar y no fallo"; \
		exit 1; \
	elif [ -e build/duplicados.dot ] || [ -e build/duplicados.svg ]; then \
		echo "ERROR: una entrada invalida no debe crear archivos de salida"; \
		exit 1; \
	else \
		echo "OK: entradas invalidas no generan salidas"; \
	fi

clean:
	rm -rf $(BUILD_DIR)
