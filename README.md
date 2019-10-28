# input

Prompt for input with readline-like key bindings.

## Motivation

I needed a program to interactively read data from standard input in a
shell script, preferably with readline-like key bindings. Back then I
didn't know about [rlwrap][rlwrap repo] and implemented this from
scratch by myself.

Contrary to rlwrap I initially used [linenoise][linenoise repo] instead
of [readline][GNU readline] as it seemed easier to use at the time.
Unfortunately, linenoise has some shortcomings and isn't properly
maintained at the moment. I therefore switched to BSD editline with
version 0.8.0.

## Features

* Support for readline-like key bindings using editline.
* Support for tab completions using arbitrary shell commands.
* Support for a persistent editing history.
* Support for wide characters.

## Usage

Just as rlwrap this program can be used to provide readline-like key
bindings for programs not linked against readline, e.g. `nc(1)`.
Additionally, it can also be used as an "alternative" to `read(1)` in
shell scripts. The provided man page contains usage examples for both
use cases.

## Installation

The following software is required:

* A C99 compiler
* [GNU make][GNU make] (sorry!)
* BSD editline / libedit (see below)

The program can be installed using:

	$ make
	$ make install

### Notes on libedit

With libedit originating from the BSD project, different versions of
that library are shipped by different BSD operating systems. I
personally use the [portable NetBSD version][NetBSD editline]. libedit
versions from other BSD operating systems may have some shortcomings.
For instance, OpenBSD [strips non-ASCII characters][openbsd nowchar] in
the `el_gets` function of their libedit version causing `input` to not
support wide characters. While `input` should compile with different
libedit versions, the test suite is only guaranteed to pass with the
NetBSD version.

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
[NetBSD editline]: https://www.thrysoee.dk/editline/
[tmux homepage]: https://tmux.github.io
[openbsd nowchar]: https://github.com/openbsd/src/blob/ddc81437857133802b1cf7d8d5bf0ff2198b602b/lib/libedit/eln.c#L77-L80
