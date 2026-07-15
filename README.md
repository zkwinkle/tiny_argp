# tiny_argp

A GNU argp inspired CLI parser for embedded / bare-metal C.

## Embedded suitability

**No allocations**

  - All state is stack-based or caller-provided.

**Minimal stdlib dependency**

  - Only `<stdbool.h>` and `<stddef.h>`
  (compiler-provided freestanding headers, always available) plus a handful
  of small routines from `<string.h>` and `<ctype.h>` (`memcpy`, `strcmp`,
  `strlen`, `isprint`, …) that any embedded libc like newlib-nano or
  picolibc supplies cheaply.

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
