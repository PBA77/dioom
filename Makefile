CC ?= cc
SDL2_PREFIX ?= /opt/homebrew/opt/sdl2
EMCC ?= emcc
EMSDK_PYTHON ?= /opt/homebrew/opt/python@3.14/bin/python3.14

CFLAGS ?= -O2 -g -Wall -Wextra -std=c11
CPPFLAGS += -I$(SDL2_PREFIX)/include
LDFLAGS += -L$(SDL2_PREFIX)/lib
LDLIBS += -lSDL2 -lm

TARGET := dioom
HIRES_TARGET := dioom-hires
HIRES_W ?= 640
HIRES_H ?= 480
HIRES_SCALE ?= 2
SRC := src/main.c
WEB_DIR := web
WEB_TARGET := $(WEB_DIR)/index.html
WEB_SHELL := $(WEB_DIR)/shell.html
WEB_ASSETS := $(shell find assets -type f)

.PHONY: all clean clean-wasm run dump hires run-hires dump-hires wasm check-emscripten

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(HIRES_TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DSCREEN_W=$(HIRES_W) -DSCREEN_H=$(HIRES_H) -DWINDOW_SCALE=$(HIRES_SCALE) -DDEFAULT_RENDER_QUALITY=RENDER_QUALITY_FAST -DDEFAULT_RENDER_EFFECTS=RENDER_EFFECTS_OFF $(LDFLAGS) -o $@ $^ $(LDLIBS)

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

$(WEB_TARGET): $(SRC) $(WEB_SHELL) $(WEB_ASSETS) | $(WEB_DIR)
	EMSDK_PYTHON="$(EMSDK_PYTHON)" $(EMCC) $(CFLAGS) -sUSE_SDL=2 -sALLOW_MEMORY_GROWTH=1 --preload-file assets@assets --shell-file $(WEB_SHELL) -o $@ $(SRC) -lm

$(WEB_DIR):
	mkdir -p $@

clean:
	rm -rf $(TARGET) $(TARGET).dSYM $(HIRES_TARGET) $(HIRES_TARGET).dSYM raycaster raycaster.dSYM raycaster-hires raycaster-hires.dSYM frame.ppm frame-hires.ppm frame.png

clean-wasm:
	rm -f $(WEB_DIR)/index.html $(WEB_DIR)/index.js $(WEB_DIR)/index.wasm $(WEB_DIR)/index.data
