# input

Prompt for input with readline-like key bindings.

## Motivation

I needed a program to interactively read data from standard input in a
shell script, preferably with readline-like key bindings. Back then I
didn't know about [rlwrap][rlwrap repo] and implemented this from
scratch by myself. Contrary to rlwrap I used a [linenoise][linenoise
repo] [fork][linenoise fork repo] instead of [readline][GNU readline]
since it seemed easier to use at the time.

## Usage

Just as rlwrap this program can be used to provide readline-like key
bindings for programs not linked against readline, e.g. `nc(1)`.
Additionally, it can also be used as an "alternative" to `read(1)` in
shell scripts. The provided man page documents usage examples for both
use cases.

## Features

* Support for readline-like key bindings using [linenoise][linenoise fork repo].
* Support for tab completions using arbitrary shell commands.
* Support for an editing history.

## Installation

The linenoise code is included in the `vendor/` subdirectory as a
git submodule since the fork is unlikely to be packaged by
distributions. Additionally, the following software is required:

	* A C99 compiler
	* [GNU make][GNU make] (sorry!)

In order to install the program the submodule needs to be initialized
and cloned. Afterwards, the program can be installed using:

	$ make
	$ make install

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
[linenoise fork repo]: https://github.com/rain-1/linenoise-mob
[GNU readline]: https://tiswww.case.edu/php/chet/readline/rltop.html
[GNU make]: https://www.gnu.org/software/make/
