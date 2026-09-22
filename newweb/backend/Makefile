CC      ?= gcc
CFLAGS  ?= -std=c17 -Wall -Wextra -Wconversion -Wshadow -Wpedantic -O2
CORE    := ../../virtual_hil
INC     := -Iinclude -I$(CORE)/include
SRC     := $(wildcard src/*.c)
CORE_SRC := $(filter-out $(CORE)/src/main.c,$(wildcard $(CORE)/src/*.c))
TARGET  := build/hil_web.exe
LIBS    := -lm

ifeq ($(OS),Windows_NT)
	LIBS += -lws2_32
else
	TARGET := build/hil_web
endif

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) $(CORE_SRC)
	@mkdir -p build
	$(CC) $(CFLAGS) $(INC) -o $@ $(SRC) $(CORE_SRC) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf build data/out
