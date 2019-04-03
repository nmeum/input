NAME = input
SOURCES = $(NAME).c complete.c
HEADERS = fns.h
OBJECTS = $(SOURCES:%.c=%.o)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/share/man
DOCDIR ?= $(PREFIX)/share/doc/$(NAME)

CFLAGS ?= -O0 -g -Werror
CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L
CFLAGS += -Wpedantic -Wall -Wextra \
	      -Wmissing-prototypes -Wpointer-arith \
	      -Wstrict-prototypes -Wshadow -Wformat-nonliteral

LDLIBS += -ledit -lncurses

$(NAME): $(OBJECTS)
%.o: %.c $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $<

format: $(SOURCES) $(HEADERS)
	clang-format -style=file -i $^

install: $(NAME) $(NAME).1 README.md
	install -Dm755 $(NAME) "$(DESTDIR)$(BINDIR)/$(NAME)"
	install -Dm644 $(NAME).1 "$(DESTDIR)$(MANDIR)/man1/$(NAME).1"
	install -Dm644 README.md "$(DESTDIR)$(DOCDIR)/README.md"

.PHONY: format install
