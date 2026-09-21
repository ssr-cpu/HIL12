CC ?= gcc

STD := c17
WARNINGS := -Wall -Wextra -Wconversion -Wshadow -Wpedantic
CPPFLAGS ?=
CFLAGS ?= -O2
CFLAGS += -std=$(STD) $(WARNINGS) -Iinclude
LDLIBS += -lm

APP_SRCS := $(wildcard src/*.c)
LIB_SRCS := $(filter-out src/main.c,$(APP_SRCS))
TEST_SRCS := $(LIB_SRCS) tests/test_main.c

ifeq ($(OS),Windows_NT)
EXE := .exe
MKDIR := powershell -NoProfile -Command "New-Item -ItemType Directory -Force -Path build"
RM_BUILD := powershell -NoProfile -Command "Remove-Item -LiteralPath 'build' -Recurse -Force -ErrorAction SilentlyContinue"
else
EXE :=
MKDIR := mkdir -p
RM_BUILD := rm -rf build
endif

APP := build/hil$(EXE)
TEST := build/hil_tests$(EXE)

.PHONY: all test sanitize clean

all: $(APP)

$(APP): $(APP_SRCS) include/*.h
	$(MKDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(APP_SRCS) $(LDLIBS) -o $@

$(TEST): $(TEST_SRCS) include/*.h
	$(MKDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(TEST_SRCS) $(LDLIBS) -o $@

test: $(TEST)
	./build/hil_tests$(EXE)

sanitize:
	$(MKDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -g $(TEST_SRCS) $(LDLIBS) -o build/hil_tests_asan$(EXE)
	./build/hil_tests_asan$(EXE)

clean:
	-$(RM_BUILD)
