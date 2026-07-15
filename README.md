# tiny_argp

A GNU argp inspired CLI parser for embedded / bare-metal C.

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
