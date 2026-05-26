CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS ?= -lm
BUILD_DIR := build
TARGET := $(BUILD_DIR)/uml_engine
SOURCES := src/main.c src/parser.c src/semantic.c src/layout.c src/route.c src/render_dot.c src/render_svg.c src/render_raylib.c src/util.c
OBJECTS := $(SOURCES:src/%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run test raylib

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
	$(TARGET) examples/juego.uml --dot build/juego.dot --svg build/juego.svg --ascii

test: $(TARGET)
	$(TARGET) examples/juego.uml --dot build/juego.dot --svg build/juego.svg --ascii
	! $(TARGET) tests/duplicados.uml --svg build/duplicados.svg

clean:
	rm -rf $(BUILD_DIR)
