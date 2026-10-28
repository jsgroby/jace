CC ?= cc
PYTHON ?= python3
MODE ?= release
TARGET := $(shell $(CC) -dumpmachine)
WINDOWS := $(or $(findstring mingw,$(TARGET)),$(findstring windows,$(TARGET)))
EXE := $(if $(WINDOWS),.exe,)
ifeq ($(strip $(WINDOWS)),)
PROGRAM = jace-baseline
BUILDROOT = build
else
ifneq ($(filter aarch64% arm64%,$(TARGET)),)
WINARCH = arm64
else ifneq ($(filter x86_64% amd64%,$(TARGET)),)
WINARCH = x64
else
$(error Unsupported Windows compiler target: $(TARGET); use ARM64 or x64)
endif
PROGRAM = jace-windows-$(WINARCH).exe
BUILDROOT = build/windows-$(WINARCH)
endif
BUILDDIR = $(BUILDROOT)/$(MODE)
WARN = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wstrict-prototypes -Wmissing-prototypes -Wformat=2
PLATFORM = $(if $(WINDOWS),-D_WIN32_WINNT=0x0600,-D_POSIX_C_SOURCE=200809L -D_DARWIN_C_SOURCE)
BASE = -I. -std=c11 $(PLATFORM) $(WARN) -MMD -MP
THREAD_FLAGS = $(if $(WINDOWS),,-pthread)
BASE += $(THREAD_FLAGS)
FLAGS_release = -O3 -DNDEBUG
FLAGS_debug = -O0 -g3 -DDEBUG
FLAGS_sanitize = -O1 -g3 -DDEBUG -fsanitize=address,undefined -fno-omit-frame-pointer
LINK_sanitize = -fsanitize=address,undefined
FLAGS_tsan = -O1 -g3 -DDEBUG -fsanitize=thread
LINK_tsan = -fsanitize=thread
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILDDIR)/%.o,$(SRCS))
.PHONY: all release debug debug-sanitize build-mode clean rebuild test smoke
all: release
release:
	$(MAKE) MODE=release build-mode
debug:
	$(MAKE) MODE=debug build-mode
debug-sanitize:
	$(MAKE) MODE=sanitize build-mode
build-mode: $(BUILDDIR)/jace$(EXE)
	cp $< $(PROGRAM)
$(BUILDDIR)/jace$(EXE): $(OBJS)
	$(CC) $(OBJS) $(THREAD_FLAGS) $(LDFLAGS) $(LINK_$(MODE)) -o $@
# Keep third-party diagnostics separate from JACE's strict warning policy.
$(BUILDDIR)/fathom.o: fathom.c $(wildcard vendor/fathom/*)
	@mkdir -p $(@D)
	$(CC) $(BASE) $(FLAGS_$(MODE)) $(CPPFLAGS) $(CFLAGS) -w -c $< -o $@
$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(BASE) $(FLAGS_$(MODE)) $(CPPFLAGS) $(CFLAGS) -c $< -o $@
-include $(OBJS:.o=.d)
clean:
	rm -rf $(BUILDROOT) $(PROGRAM)
rebuild:
	$(MAKE) clean
	$(MAKE) all
test:
	$(PYTHON) tests/run.py
smoke: release
	$(MAKE) thread-test
	$(MAKE) shared-test smp-failure-test
	$(PYTHON) tests/portable_smoke.py ./$(PROGRAM)
	$(PYTHON) tests/smp_smoke.py ./$(PROGRAM)
	$(PYTHON) tests/syzygy_smoke.py ./$(PROGRAM)
	$(PYTHON) tests/syzygy_smoke.py ./$(PROGRAM) "$(TABLES)" 4

.PHONY: thread-test thread-tsan
$(BUILDDIR)/thread-tests$(EXE): threads.c threads.h tests/threads.c
	@mkdir -p $(@D)
	$(CC) $(BASE) $(FLAGS_$(MODE)) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -DJACE_THREAD_TESTING threads.c tests/threads.c $(THREAD_FLAGS) $(LDFLAGS) $(LINK_$(MODE)) -o $@
thread-test: $(BUILDDIR)/thread-tests$(EXE)
	$(PYTHON) tests/thread_smoke.py ./$(BUILDDIR)/thread-tests$(EXE)
thread-tsan:
	@mkdir -p $(BUILDROOT)/tsan
	$(CC) $(BASE) -O1 -g -UNDEBUG -DJACE_THREAD_TESTING -fsanitize=thread threads.c tests/threads.c $(THREAD_FLAGS) -fsanitize=thread -o $(BUILDROOT)/tsan/thread-tests$(EXE)
	$(PYTHON) tests/thread_smoke.py ./$(BUILDROOT)/tsan/thread-tests$(EXE)

SHARED_OBJECTS = $(filter-out $(BUILDDIR)/jace.o,$(OBJS))
TABLES ?= tests/syzygy-data
.PHONY: shared-test shared-tsan
$(BUILDDIR)/shared-tests$(EXE): $(SHARED_OBJECTS) $(wildcard *.h) tests/shared.c
	@mkdir -p $(@D)
	$(CC) $(BASE) $(FLAGS_$(MODE)) $(CPPFLAGS) $(CFLAGS) -UNDEBUG tests/shared.c $(SHARED_OBJECTS) $(THREAD_FLAGS) $(LDFLAGS) $(LINK_$(MODE)) -o $@
shared-test: $(BUILDDIR)/shared-tests$(EXE)
	$(PYTHON) tests/thread_smoke.py ./$(BUILDDIR)/shared-tests$(EXE) "$(TABLES)"
shared-tsan:
	$(MAKE) MODE=tsan shared-test

.PHONY: smp-tsan
smp-tsan:
	$(MAKE) MODE=tsan build-mode smp-failure-test
	$(PYTHON) tests/smp_smoke.py ./$(BUILDROOT)/tsan/jace$(EXE)
	$(PYTHON) tests/syzygy_smoke.py ./$(BUILDROOT)/tsan/jace$(EXE) "$(TABLES)" 4

SMP_TEST_OBJECTS = $(filter-out $(BUILDDIR)/jace.o $(BUILDDIR)/search.o $(BUILDDIR)/threads.o,$(OBJS))
.PHONY: smp-failure-test
$(BUILDDIR)/smp-failure-tests$(EXE): $(SMP_TEST_OBJECTS) search.c threads.c threads.h tests/smp_failure.c
	$(CC) $(BASE) $(FLAGS_$(MODE)) $(CPPFLAGS) $(CFLAGS) -UNDEBUG -DJACE_THREAD_TESTING tests/smp_failure.c threads.c $(SMP_TEST_OBJECTS) $(THREAD_FLAGS) $(LDFLAGS) $(LINK_$(MODE)) -o $@
smp-failure-test: $(BUILDDIR)/smp-failure-tests$(EXE)
	$(PYTHON) tests/thread_smoke.py ./$<
