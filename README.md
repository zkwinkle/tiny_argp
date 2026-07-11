# tiny_argp

A GNU argp inspired CLI parser for embedded / bare-metal C.

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
