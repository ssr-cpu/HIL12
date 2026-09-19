CC ?= gcc
CFLAGS ?= -O2
CFLAGS += -std=c17 -Wall -Wextra -Wconversion -Wshadow -Wpedantic
CPPFLAGS += -Iinclude

ifeq ($(OS),Windows_NT)
LDLIBS += -lws2_32
EXE := .exe
else
LDLIBS :=
EXE :=
endif

TARGET := build/web_server$(EXE)
SOURCES := $(wildcard src/*.c)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SOURCES) include/web_server.h
	mkdir -p build
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCES) $(LDLIBS) -o $@

run: $(TARGET)
	cd .. && backend/$(TARGET)

clean:
	rm -rf build
