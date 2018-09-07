NAME = input

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
DOCDIR ?= $(PREFIX)/share/doc/$(NAME)

CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra -Wconversion \
	      -Wmissing-prototypes -Wpointer-arith \
	      -Wstrict-prototypes -Wshadow -Wformat-nonliteral

CPPFLAGS += -I./vendor/linenoise

input: $(NAME).o liblinenoise.a
format: .clang-format $(NAME).c
	clang-format -style=file -i $(NAME).c

linenoise.o: linenoise.c linenoise.h
	$(CC) -c $< -o $@ $(CFLAGS) -w
liblinenoise.a: linenoise.o
	$(AR) rcs $@ $^

install: $(NAME) $(NAME).1 README.md
	install -Dm755 $(NAME) "$(DESTDIR)$(BINDIR)/$(NAME)"
	install -Dm644 $(NAME).1 "$(DESTDIR)$(MANDIR)/man1/$(NAME).1"
	install -Dm644 README.md "$(DESTDIR)$(DOCDIR)/README.md"

VPATH += vendor/linenoise
.PHONY: format install
