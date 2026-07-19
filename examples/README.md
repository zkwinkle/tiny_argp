# tiny_argp examples

Self-contained programs that build against the library in the parent directory.

- [ex1_minimal.c](ex1_minimal.c): A minimal program with no options and no
positional handling. Only has `--help` and `--usage`, which are added by the
library.

- [ex2_options.c](ex2_options.c): A program with a handful of options and no
positional arguments. Uses a `struct arguments` passed through `state->input`
to communicate between `main()` and the parser callback.

- [ex3_positionals.c](ex3_positionals.c): Adds a required positional argument
`ARG1` and a variable-length list of trailing `STRING`s on top of ex2's
options. Also has more advanced `--help` output formatting through `docs` and
`arg_docs`.

- [ex4_subcommands.c](examples/ex4_subcommands.c): An example using `git`-style
subcommands. An initial parser looks for an `add` or `list` subcommands and
then calls their parsers with the remaining arguments. Each subcommand has its
own options and help output.

## Build

```
make
```

Binaries are placed under `build/`.

## Run

```
build/ex1_minimal --help
build/ex2_options -v -o /tmp/out
build/ex3_positionals -v -o /tmp/out A B C
build/ex4_subcommands add --name=widget --count=3
```

## Verify

```
make verify
```

Runs [verify.sh](verify.sh), a shell harness that exercises each example with
a spread of argument combinations and asserts on exit code and output.
