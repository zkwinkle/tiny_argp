# tiny_argp

A GNU argp inspired CLI parser for embedded / bare-metal C.

## Usage

Copy [tiny_argp.c](./tiny_argp.c) and [tiny_argp.h](./tiny_argp.h) into your
project and build them alongside your other sources.

To get started, look at [examples/ex1_minimal.c](examples/ex1_minimal.c) for a
minimal working CLI program, then browse the rest of the [examples](#examples)
below for options, positional arguments, and subcommands.

## Embedded suitability

**No allocations**

  - All state is stack-based or caller-provided.

**Minimal stdlib dependency**

  - Only `<stdbool.h>` and `<stddef.h>`
  (compiler-provided freestanding headers, always available) plus a handful
  of small routines from `<string.h>` and `<ctype.h>` (`memcpy`, `strcmp`,
  `strlen`, `isprint`, …) that an embedded libc like newlib-nano or
  picolibc supplies.

**Configurable output**

  - All output goes through `printer` / `err_printer` callbacks, so you provide
    your own `printf` and route it to whichever channel your target uses (UART,
    USB serial, stdout, an in-memory log buffer, etc.). See [Printer
    requirements](#printer-requirements) below.

**Never terminates the runtime**

  - No `exit()` or similar, errors are returned to the caller.

## Printer requirements

All output goes through the `printer` and `err_printer` callbacks on the
`tiny_argp` struct. They have a `printf`-style signature:

```c
typedef int (*tiny_argp_printer_t)(const char *fmt, ...);
```

The library only ever calls them with the following conversion specifiers:

- `%s`: null-terminated string
- `%c`: single character

Any minimal `printf` implementation that handles `%s` and `%c` is sufficient
(e.g. [mpaland/printf](https://github.com/mpaland/printf), [nanoprintf](https://github.com/charlesnicholson/nanoprintf)).

## Examples

The [examples/](./examples/) directory contains four self-contained programs.

- [examples/ex1_minimal.c](examples/ex1_minimal.c): A minimal program with no
options and no positional handling. Only has `--help` and `--usage` which are
added by the library.
- [examples/ex2_options.c](examples/ex2_options.c): A program with a handful
of options and no positional arguments.
- [examples/ex3_positionals.c](examples/ex3_positionals.c): Adds a required
positional argument `ARG1` and a variable-length list of trailing `STRING`s on
top of ex2's options. Also has more advanced `--help` output formatting.
- [examples/ex4_subcommands.c](examples/ex4_subcommands.c): An example using
`git`-style subcommands. An initial parser looks for an `add` or `list`
subcommands and then calls their parsers with the remaining arguments. Each
subcommand has its own options and help output.

Build them all with:

```
make -C examples
```

Then invoke any of the binaries directly:

```
examples/build/ex2_options --help
examples/build/ex3_positionals -v -o /tmp/out A B C
examples/build/ex4_subcommands add --name=widget --count=3
```

`make -C examples verify` runs a shell harness that exercises each example with
a spread of arg combinations and asserts on exit code and output.

## To Test

```
make -C tests run
```

Run the test suite with `-O0`, `-Os`, `-O2`, and `-O3`:

```
make -C tests matrix
```

## Check binary size

Prints the `text` / `data` / `bss` sizes of the compiled object at the
current `CFLAGS`:

```
make size
```

Override optimization level to compare, e.g.:

```
make clean
make size CFLAGS="-std=c11 -O2"
```

As a rough reference, on x86_64 with GCC 16.1 the compiled object is around
~12.5 kB with `-O0`, and ~6.5 kB with `-Os`.

## AI usage

All library code was written by hand.

AI assistance was limited to writing unit tests (reviewed and edited by hand)
and copyediting the docs.
