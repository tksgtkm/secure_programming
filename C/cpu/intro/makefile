CXX	?= g++
CC  ?= gcc

TARGET   ?= app

# --- sources ---
CXX_SRCS := main.cc misc.cc
ASM_SRCS := intro.s
OBJS     := $(CXX_SRCS:.cc=.o) $(ASM_SRCS:.s=.o)
DEPS     := $(CXX_SRCS:.cc=.d)

# --- flags ---
CPPFLAGS ?=
CXXFLAGS ?= -std=c++20 -O2 -Wall -Wextra -pedantic -g -MMD -MP
AS = nasm
ASFLAGS  ?= -f elf64
LDFLAGS  ?=

# --- default ---
.PHONY: all
all: $(TARGET)

# --- link ---
$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# --- compile (C++) ---
%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# --- assemble (.s -> .o) ---
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# --- run helper ---
.PHONY: run
run: $(TARGET)
	./$(TARGET)

# --- clean ---
.PHONY: clean
clean:
	$(RM) $(TARGET) $(OBJS) $(DEPS)

# --- include auto-deps ---
-include $(DEPS)