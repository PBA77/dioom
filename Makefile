CC ?= cc
SDL2_PREFIX ?= /opt/homebrew/opt/sdl2

CFLAGS ?= -O2 -g -Wall -Wextra -std=c11
CPPFLAGS += -I$(SDL2_PREFIX)/include
LDFLAGS += -L$(SDL2_PREFIX)/lib
LDLIBS += -lSDL2 -lm

TARGET := raycaster
SRC := src/main.c

.PHONY: all clean run dump

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

dump: $(TARGET)
	./$(TARGET) --dump frame.ppm

clean:
	rm -rf $(TARGET) $(TARGET).dSYM frame.ppm frame.png
