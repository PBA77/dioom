CC ?= cc
SDL2_PREFIX ?= /opt/homebrew/opt/sdl2
EMCC ?= emcc
EMSDK_PYTHON ?= /opt/homebrew/opt/python@3.14/bin/python3.14

CFLAGS ?= -O2 -g -Wall -Wextra -std=c11
CFLAGS += -flto
CPPFLAGS += -I$(SDL2_PREFIX)/include
LDFLAGS += -L$(SDL2_PREFIX)/lib
LDLIBS += -lSDL2 -lm

TARGET := dioom
HIRES_TARGET := dioom-hires
HIRES_W ?= 640
HIRES_H ?= 480
HIRES_SCALE ?= 2
SRC := $(wildcard src/*.c)
HEADERS := $(wildcard src/*.h)
OBJ := $(patsubst src/%.c,build/%.o,$(SRC))
WEB_DIR := web
WEB_TARGET := $(WEB_DIR)/index.html
WEB_SHELL := $(WEB_DIR)/shell.html
WEB_ASSETS := $(shell find assets -type f)
WEB_SETTINGS := $(WEB_DIR)/dioom.ini

.PHONY: all clean clean-wasm run dump hires run-hires dump-hires wasm check-emscripten

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c $(HEADERS)
	@mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(OBJ:.o=.d)

$(HIRES_TARGET): $(SRC) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSCREEN_W=$(HIRES_W) -DSCREEN_H=$(HIRES_H) -DWINDOW_SCALE=$(HIRES_SCALE) -DDEFAULT_RENDER_QUALITY=RENDER_QUALITY_FAST -DDEFAULT_RENDER_EFFECTS=RENDER_EFFECTS_OFF $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

dump: $(TARGET)
	./$(TARGET) --dump frame.ppm

hires: $(HIRES_TARGET)

run-hires: $(HIRES_TARGET)
	./$(HIRES_TARGET) --scale linear --quality fast --effects off

dump-hires: $(HIRES_TARGET)
	./$(HIRES_TARGET) --dump frame-hires.ppm

wasm: check-emscripten $(WEB_TARGET)

check-emscripten:
	@if ! command -v "$(EMCC)" >/dev/null 2>&1; then \
		echo "error: emcc not found. Install Emscripten or run make wasm EMCC=/absolute/path/to/emcc" >&2; \
		exit 1; \
	fi
	@if [ ! -x "$(EMSDK_PYTHON)" ]; then \
		echo "error: Emscripten Python not found: $(EMSDK_PYTHON). Run make wasm EMSDK_PYTHON=/absolute/path/to/python3.10-or-newer" >&2; \
		exit 1; \
	fi

$(WEB_TARGET): $(SRC) $(HEADERS) $(WEB_SHELL) $(WEB_SETTINGS) $(WEB_ASSETS) | $(WEB_DIR)
	EMSDK_PYTHON="$(EMSDK_PYTHON)" $(EMCC) $(filter-out -flto,$(CFLAGS)) -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 --preload-file assets@assets --preload-file $(WEB_SETTINGS)@dioom.ini --shell-file $(WEB_SHELL) -o $@ $(SRC) -lm

$(WEB_DIR):
	mkdir -p $@

clean:
	rm -rf build
	rm -rf $(TARGET) $(TARGET).dSYM $(HIRES_TARGET) $(HIRES_TARGET).dSYM raycaster raycaster.dSYM raycaster-hires raycaster-hires.dSYM frame.ppm frame-hires.ppm frame.png

clean-wasm:
	rm -f $(WEB_DIR)/index.html $(WEB_DIR)/index.js $(WEB_DIR)/index.wasm $(WEB_DIR)/index.data
