NAME = input

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
DOCDIR ?= $(PREFIX)/share/doc/$(NAME)

CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra \
	      -Wmissing-prototypes -Wpointer-arith \
	      -Wstrict-prototypes -Wshadow -Wformat-nonliteral

LDLIBS = -lreadline -lncurses

$(NAME): $(NAME).c
check: $(NAME)
	cd tests/ && ./run_tests.sh

format: $(SOURCES)
	clang-format -style=file -i $(NAME).c

install: $(NAME) $(NAME).1 README.md
	install -Dm755 $(NAME) "$(DESTDIR)$(BINDIR)/$(NAME)"
	install -Dm644 $(NAME).1 "$(DESTDIR)$(MANDIR)/man1/$(NAME).1"
	install -Dm644 README.md "$(DESTDIR)$(DOCDIR)/README.md"

.PHONY: check format install
