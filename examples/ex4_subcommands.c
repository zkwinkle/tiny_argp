/* Example #4: git-style sub-commands.
 *
 * The program dispatches to one of two sub-commands based on the first
 * positional argument:
 *
 *   ex4_subcommands add --name=widget --count=3
 *   ex4_subcommands list -v
 *   ex4_subcommands --help
 *
 * There is one tiny_argp parser at the top level plus one more per
 * sub-command. The top-level parser has no options of its own; it only
 * needs to catch the first positional argument. TINY_ARGP_IN_ORDER
 * keeps that positional in its original place so options belonging to
 * the sub-command are not pulled forward past it.
 *
 * When the top-level parser sees TINY_ARGP_KEY_ARG it looks the token
 * up in the sub-command table and hands the tail of argv to the
 * matching parser. state->next points just past the sub-command name,
 * so &state->argv[state->next - 1] and state->argc - (state->next - 1)
 * define the slice. Before dispatching, the code overwrites
 * argv[state->next - 1] with "<prog> <sub>" so any error messages
 * printed by the nested parser identify which sub-command produced them.
 * Once the sub-command returns, setting state->next = state->argc tells
 * the outer parser that everything else has already been consumed.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tiny_argp.h"

static int fprintf_stderr(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int r = vfprintf(stderr, fmt, ap);
  va_end(ap);
  return r;
}

/* --- 'add' sub-command ---------------------------------------------- */

struct add_args {
  const char *name;
  int count;
};

static const struct tiny_argp_option add_options[] = {
    {"name", 'n', "NAME", 0, "Name of the item to add"},
    {"count", 'c', "N", 0, "How many to add (default 1)"},
    {0}};

static int add_parse(int key, char *arg, struct tiny_argp_state *state) {
  struct add_args *a = state->input;
  switch (key) {
  case TINY_ARGP_KEY_INIT:
    a->name = NULL;
    a->count = 1;
    break;
  case 'n':
    a->name = arg;
    break;
  case 'c':
    a->count = atoi(arg);
    break;
  case TINY_ARGP_KEY_END:
    if (!a->name) {
      tiny_argp_error(state, "--name is required");
      return TINY_ARGP_ERR_PARSE;
    }
    break;
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_FINI:
    break;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
  return TINY_ARGP_SUCCESS;
}

static const struct tiny_argp add_argp = {
    .options = add_options,
    .parser = add_parse,
    .args_doc = NULL,
    .doc = "Add an item to the collection.",
    .printer = printf,
    .err_printer = fprintf_stderr,
};

static int add_run(int argc, char **argv) {
  struct add_args args;
  int status = tiny_argp_parse(&add_argp, argc, argv, 0, NULL, &args);
  if (status != TINY_ARGP_SUCCESS)
    return status;
  printf("adding %d copies of '%s'\n", args.count, args.name);
  return 0;
}

/* --- 'list' sub-command --------------------------------------------- */

struct list_args {
  int verbose;
};

static const struct tiny_argp_option list_options[] = {
    {"verbose", 'v', 0, 0, "Include hidden items"},
    {0}};

static int list_parse(int key, char *arg, struct tiny_argp_state *state) {
  struct list_args *a = state->input;
  (void)arg;
  switch (key) {
  case TINY_ARGP_KEY_INIT:
    a->verbose = 0;
    break;
  case 'v':
    a->verbose = 1;
    break;
  case TINY_ARGP_KEY_ARG:
  case TINY_ARGP_KEY_NO_ARGS:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_FINI:
    break;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
  return TINY_ARGP_SUCCESS;
}

static const struct tiny_argp list_argp = {
    .options = list_options,
    .parser = list_parse,
    .args_doc = NULL,
    .doc = "List items in the collection.",
    .printer = printf,
    .err_printer = fprintf_stderr,
};

static int list_run(int argc, char **argv) {
  struct list_args args;
  int status = tiny_argp_parse(&list_argp, argc, argv, 0, NULL, &args);
  if (status != TINY_ARGP_SUCCESS)
    return status;
  printf("listing items (verbose=%s)\n", args.verbose ? "yes" : "no");
  return 0;
}

/* --- top-level dispatcher ------------------------------------------ */

struct subcommand {
  const char *name;
  int (*run)(int argc, char **argv);
};

static const struct subcommand subcommands[] = {
    {"add", add_run},
    {"list", list_run},
    {0},
};

/* Storage for the synthesised "<prog> <sub>" program name so that error
 * output from the nested parse identifies the sub-command. */
static char compound_name[64];

static int top_parse(int key, char *arg, struct tiny_argp_state *state) {
  switch (key) {
  case TINY_ARGP_KEY_ARG: {
    const struct subcommand *sub = subcommands;
    while (sub->name && strcmp(sub->name, arg) != 0)
      sub++;
    if (!sub->name) {
      tiny_argp_error(state, "unknown sub-command");
      return TINY_ARGP_ERR_PARSE;
    }
    snprintf(compound_name, sizeof compound_name, "%s %s", state->name, arg);
    state->argv[state->next - 1] = compound_name;
    int rc = sub->run((int)(state->argc - (state->next - 1)),
                      &state->argv[state->next - 1]);
    state->next = state->argc;
    if (rc != 0)
      return rc;
    break;
  }
  case TINY_ARGP_KEY_NO_ARGS:
    tiny_argp_usage(state);
    return TINY_ARGP_ERR_PARSE;
  case TINY_ARGP_KEY_INIT:
  case TINY_ARGP_KEY_END:
  case TINY_ARGP_KEY_SUCCESS:
  case TINY_ARGP_KEY_FINI:
    break;
  default:
    return TINY_ARGP_ERR_UNKNOWN;
  }
  return TINY_ARGP_SUCCESS;
}

static const struct tiny_argp_option top_options[] = {{0}};

static const struct tiny_argp top_argp = {
    .options = top_options,
    .parser = top_parse,
    .args_doc = "SUBCOMMAND [ARGS...]",
    .doc = "Sub-commands: add, list.  Pass --help after a sub-command name "
           "for its options.",
    .printer = printf,
    .err_printer = fprintf_stderr,
};

int main(int argc, char **argv) {
  return tiny_argp_parse(&top_argp, argc, argv, TINY_ARGP_IN_ORDER, NULL, NULL);
}
