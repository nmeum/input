SOURCES = $(wildcard *.c)

CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra -Wconversion \
	      -Wmissing-prototypes -Wpointer-arith \
	      -Wstrict-prototypes -Wshadow -Wformat-nonliteral

CPPFLAGS += -I./vendor/linenoise

ifeq "$(findstring clang,$(shell $(CC) --version))" "clang"
	CFLAGS += -Weverything -Wno-disabled-macro-expansion
endif

input: input.o liblinenoise.a

linenoise.o: linenoise.c linenoise.h
	$(CC) -c $< -o $@ $(CFLAGS) -w
liblinenoise.a: linenoise.o
	$(AR) rcs $@ $^

format: .clang-format $(SOURCES)
	clang-format -style=file -i $(SOURCES)

VPATH += vendor/linenoise
