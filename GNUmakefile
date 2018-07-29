SOURCES = $(wildcard *.c)

CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra -Wconversion -Wmissing-prototypes \
	-Wpointer-arith -Wstrict-prototypes -Wshadow

CPPFLAGS += -I./vendor/linenoise

ifeq "$(findstring clang,$(shell $(CC) --version))" "clang"
	CFLAGS += -Weverything
endif

input: input.c liblinenoise.a

linenoise.c utf8.c: linenoise.h
liblinenoise.a: linenoise.c utf8.c
	$(CC) -c $^ -O0 -std=c99 -D_POSIX_C_SOURCE=200809L -w
	$(AR) rcs $@ linenoise.o utf8.o

format: .clang-format $(SOURCES)
	clang-format -style=file -i $(SOURCES)

VPATH += vendor/linenoise
