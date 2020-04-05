# input

Prompt for input with readline-like key bindings.

## Motivation

I needed a program to interactively read data from standard input in a
shell script, preferably with readline-like key bindings. Back then I
didn't know about [rlwrap][rlwrap repo] and implemented this from
scratch by myself. Contrary to rlwrap, this implementation focuses on
shell scripting and not on providing a "readline wrapper" for existing
applications. As an additional difference, input initially used a fork
of the [linenoise][linenoise repo] editing library.  Since linenoise
wasn't properly maintained, later version switched to BSD editline and
GNU readline. The last version, released with linenoise support, is
`0.7.2`.

## Features

* Support for readline-like key bindings.
* Support for tab completions using arbitrary commands.
* Support for a persistent editing history.

## Installation

The following software is required:

* A C99 compiler
* [GNU make][GNU make] (sorry!)
* [GNU readline][GNU readline] (or BSD editline)

The program can be installed using:

	$ make install

On BSD using editline instead of readline is possible by installing using:

	$ gmake LDLIBS="-ledit -lncurses" install

## Testing

A test suite using [tmux][tmux homepage] is also available. The test
suite relies on heuristics to determine whether the process under test
is ready to receive input and might thus be a bit racy. It can be
invoked using:

	$ make check

## License

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation, either version 3 of the License, or (at your
option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <http://www.gnu.org/licenses/>.

[rlwrap repo]: https://github.com/hanslub42/rlwrap
[linenoise repo]: https://github.com/antirez/linenoise
[GNU readline]: https://tiswww.case.edu/php/chet/readline/rltop.html
[GNU make]: https://www.gnu.org/software/make/
[tmux homepage]: https://tmux.github.io
