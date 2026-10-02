CC ?= cc
PYTHON ?= python3
MODE ?= release
TARGET := $(shell $(CC) -dumpmachine)
WINDOWS := $(or $(findstring mingw,$(TARGET)),$(findstring windows,$(TARGET)))
EXE := $(if $(WINDOWS),.exe,)
PROGRAM = jace-baseline$(EXE)
WARN = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2
PLATFORM = $(if $(WINDOWS),-D_WIN32_WINNT=0x0600,-D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE)
BASE = -std=c11 $(PLATFORM) $(WARN) -MMD -MP
FLAGS_release = -O3 -DNDEBUG
FLAGS_debug = -O0 -g3 -DDEBUG
FLAGS_sanitize = -O1 -g3 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
LINK_sanitize = -fsanitize=address,undefined
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,build/$(MODE)/%.o,$(SRCS))
.PHONY: all release debug debug-sanitize build-mode clean rebuild test smoke
all: release
release:
	$(MAKE) MODE=release build-mode
debug:
	$(MAKE) MODE=debug build-mode
debug-sanitize:
	$(MAKE) MODE=sanitize build-mode
build-mode: build/$(MODE)/jace$(EXE)
	cp $< $(PROGRAM)
build/$(MODE)/jace$(EXE): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) $(LINK_$(MODE)) -o $@
build/$(MODE)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(BASE) $(FLAGS_$(MODE)) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
-include $(OBJS:.o=.d)
clean:
	rm -rf build jace-baseline jace-baseline.exe
rebuild:
	$(MAKE) clean
	$(MAKE) all
test:
	$(PYTHON) tests/run.py
smoke: release
	$(PYTHON) tests/portable_smoke.py ./$(PROGRAM)
