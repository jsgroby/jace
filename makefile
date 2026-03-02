# ===== Project =====
NAME   := jace
SRCS   := $(wildcard *.c)
OBJS   := $(SRCS:.c=.o)
DEPS   := $(OBJS:.o=.d)

# Compiler / tools
CC     ?= gcc

# ===== Flags =====
STD    := -std=c11

WARN   := -Wall -Wextra -Wpedantic \
          -Wshadow -Wconversion -Wsign-conversion \
          -Wstrict-prototypes -Wmissing-prototypes -Wformat=2

# Generate dependency files alongside .o files
DEPFLAGS := -MMD -MP

# Common flags for all builds
CFLAGS  ?= $(STD) $(WARN) $(DEPFLAGS)
LDFLAGS ?=

# ===== Build modes =====
RELEASE_CFLAGS := -O3 -DNDEBUG -flto
RELEASE_LDFLAGS := -flto

DEBUG_CFLAGS   := -O0 -g3
# Sanitizers are incredibly useful for a C engine (UB/ASan finds real bugs fast)
SANITIZERS     := -fsanitize=address,undefined
DEBUG_SAN_CFLAGS := $(SANITIZERS) -fno-omit-frame-pointer
DEBUG_SAN_LDFLAGS := $(SANITIZERS)

# ===== Default target =====
.PHONY: all
all: release

# ===== Targets =====
.PHONY: release
release: CFLAGS  += $(RELEASE_CFLAGS)
release: LDFLAGS += $(RELEASE_LDFLAGS)
release: $(NAME)

.PHONY: debug
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(NAME)

.PHONY: debug-sanitize
debug-sanitize: CFLAGS  += $(DEBUG_CFLAGS) $(DEBUG_SAN_CFLAGS)
debug-sanitize: LDFLAGS += $(DEBUG_SAN_LDFLAGS)
debug-sanitize: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Compile .c -> .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Pull in auto-generated dependency files (ignore if missing)
-include $(DEPS)

# ===== Utilities =====
.PHONY: clean
clean:
	rm -f $(NAME) $(OBJS) $(DEPS)

.PHONY: rebuild
rebuild: clean all