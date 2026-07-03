SHELL := /bin/sh

TARGET := build/dioom060
SRC := main.c
P96_INCLUDE := p96include

M68K_GCC := $(shell command -v m68k-amigaos-gcc 2>/dev/null || command -v m68k-amigaos-gcc-13 2>/dev/null || command -v m68k-amigaos-gcc-12 2>/dev/null)
VC_BIN := $(shell command -v vc 2>/dev/null)
VC_IS_VERCEL := $(shell if [ -n "$(VC_BIN)" ] && "$(VC_BIN)" --version 2>&1 | grep -qi 'vercel'; then echo yes; fi)

CC_AMIGA ?= $(if $(M68K_GCC),$(M68K_GCC),$(if $(VC_IS_VERCEL),,$(VC_BIN)))
TOOLCHAIN := $(if $(M68K_GCC),gcc,$(if $(CC_AMIGA),vbcc,missing))

GCC_CFLAGS ?= -O3 -m68020-60 -msoft-float -fomit-frame-pointer -Wall -Wextra -std=c99 -noixemul -I$(P96_INCLUDE)
GCC_LDLIBS ?= -lamiga

VBCC_TARGET ?= +aos68k
VBCC_CFLAGS ?= -O2 -cpu=68000 -c99 -I$(P96_INCLUDE)
VBCC_LDLIBS ?= -lamiga

.PHONY: all preflight clean print-toolchain winuae-config

all: preflight $(TARGET)

preflight:
	@if [ "$(TOOLCHAIN)" = "missing" ]; then \
		echo "error: no AmigaOS m68k C toolchain found." >&2; \
		echo "Install or expose m68k-amigaos-gcc, or vbcc 'vc' with an AmigaOS m68k target." >&2; \
		echo "Refusing to use m68k-elf-gcc or unrelated 'vc' commands." >&2; \
		if [ -n "$(VC_BIN)" ] && [ "$(VC_IS_VERCEL)" = "yes" ]; then \
			echo "note: found $(VC_BIN), but it is Vercel CLI, not vbcc." >&2; \
		fi; \
		exit 1; \
	fi

print-toolchain:
	@echo "toolchain=$(TOOLCHAIN)"
	@echo "compiler=$(CC_AMIGA)"

$(TARGET): $(SRC) wall_textures.h p96include/proto/Picasso96.h p96include/libraries/Picasso96.h | build
ifeq ($(TOOLCHAIN),gcc)
	$(CC_AMIGA) $(GCC_CFLAGS) -o $@ $(SRC) $(GCC_LDLIBS)
else ifeq ($(TOOLCHAIN),vbcc)
	$(CC_AMIGA) $(VBCC_TARGET) $(VBCC_CFLAGS) -o $@ $(SRC) $(VBCC_LDLIBS)
else
	@false
endif

build:
	mkdir -p $@

winuae-config:
	@printf '%s\n' "Use winuae/dioom060.uae as the local test config."
	@printf '%s\n' "It mounts this directory as DIOOM060: and expects $(TARGET) after a successful build."

clean:
	rm -rf build
