#include "tiny_argp.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/*****************************************************************************
 * Macros
 ****************************************************************************/
#define PSTATE(state_ptr) ((struct pstate *)(state_ptr->pstate))
#define EXIT_STATE(state_ptr) PSTATE(state_ptr)->force_exit
#define EXIT_CODE(state_ptr) PSTATE(state_ptr)->exit_code

#define LINE_LENGTH 80

/*****************************************************************************
 * Local enum types
 ****************************************************************************/
enum arg_dashes { SHORT = 1, LONG = 2 };

enum exit_type {
  NO_EXIT = 0,          // Don't exit
  IMMEDIATE_EXIT = 1,   // Exit immediately
  ERROR_THEN_EXIT = 2,  // Pass ERROR and FINI keys to parse before exiting
  GOTO_END = 4,         // Skip to END check then continue normally
};

/*****************************************************************************
 * Local struct types
 ****************************************************************************/
struct pstate {
  enum exit_type force_exit;  // For when something causes an early exit
  int exit_code;              // Value to return on early exit/error/whatever
  bool no_args;    // Keep track if any non-option arguments have been parsed
  size_t arg_num;  // For overwriting the public state's arg_num value
  size_t quote_index;      // Keeping track of '--' quote arg
  char *current_opt_name;  // The opt as it was given in the command line
  bool manual_exit;        // Special flag for if tiny_argp_exit() is called
};

struct opt_arg {
  char *opt;
  const struct tiny_argp_option *option;
  char *arg;
  const enum arg_dashes size;
};

/*****************************************************************************
 * Help options reserved by
 ****************************************************************************/
// The long names and the '?' key are reserved.
// It's okay if the user uses the -1 key.
// It's important to mantain the order of the help options (see process_option).
static const struct tiny_argp_option help_options[] = {
    {"help", '?', 0, 0, "Give this help list"},
    {"usage", -1, 0, 0, "Give a short usage message"},
    {0}};

/*****************************************************************************
 * Forward declarations
 ****************************************************************************/
void tiny_argp_help(const struct tiny_argp *tiny_argp,
                  const tiny_argp_printer_t printer, unsigned flags, char *name);

bool is_end_option(const struct tiny_argp_option tiny_argp);

/*****************************************************************************
 * Printing function definitions
 ****************************************************************************/
// Format string should always include the name of the command and the option
// in which the error ocurred in that order.
static void opt_error(const char *format, int error, const char *opt,
                      const struct tiny_argp_state *state) {
  const tiny_argp_printer_t err_printer = state->root_tiny_argp->err_printer;
  if (!(state->flags & TINY_ARGP_NO_ERRS)) {
    err_printer(format, state->name, opt);
    tiny_argp_help(state->root_tiny_argp, state->root_tiny_argp->err_printer,
                 TINY_ARGP_HELP_STD_ERR, state->name);
  }
  if (state->flags & TINY_ARGP_NO_EXIT) {
    EXIT_STATE(state) = ERROR_THEN_EXIT;
  } else {
    EXIT_STATE(state) = IMMEDIATE_EXIT;
  }
  EXIT_CODE(state) = error;
}

#define USAGE_INDENT 12
static void usage_newline_if_eol(size_t *current_col, size_t to_print,
                                 const tiny_argp_printer_t printer) {
  // reserve last column for newline char
  if ((*current_col + to_print) >= LINE_LENGTH - 1) {
    printer("\r\n%*s", USAGE_INDENT, "");
    *current_col = USAGE_INDENT;
  }
}

static void print_opt_usage(enum arg_dashes dashes, struct tiny_argp_option opt,
                            size_t *current_col,
                            const tiny_argp_printer_t printer) {
  // Early exit conditions
  if (opt.flags & OPTION_HIDDEN) {
    return;
  }
  // Short options with no arguments are handled specially by caller function
  if (dashes == SHORT && (!isprint(opt.key) || opt.arg == 0)) {
    return;
  }
  if (dashes == LONG && opt.name == 0) {
    return;
  }

  // First we must calculate the length of the string to print
  size_t print_length = 2 + dashes;  // Start with 2 for braces [] + dashes
  if (opt.arg != 0) {
    print_length += strlen(opt.arg) + 1;  // +1 for space/equal
    if (opt.flags & OPTION_ARG_OPTIONAL) {
      // Another 2 braces around arg (but only adds 1 char in total for SHORT)
      print_length += dashes;
    }
  }
  if (dashes == LONG) {
    print_length += strlen(opt.name);
  } else {
    print_length += 1;
  }

  usage_newline_if_eol(current_col, print_length, printer);
  *current_col += print_length;

  // The reason for covering each branching case specifically is to avoid
  // multiple calls to printer. There could be less nested logic by setting up
  // a char buffer and filling it char by char, since we already know the size
  // of the string we will print (print_length). But this is more
  // straightforward and is 100% proof to a buffer overflow bug.
  if (dashes == LONG) {
    if (opt.arg != 0) {
      if (opt.flags & OPTION_ARG_OPTIONAL) {
        printer("[--%s=[%s]]", opt.name, opt.arg);
      } else {
        printer("[--%s=%s]", opt.name, opt.arg);
      }
    } else {
      printer("[--%s]", opt.name);
    }
  } else {
    if (opt.arg != 0) {
      if (opt.flags & OPTION_ARG_OPTIONAL) {
        printer("[-%c[%s]]", opt.key, opt.arg);
      } else {
        printer("[-%c %s]", opt.key, opt.arg);
      }
    } else {
      printer("[-%c]", opt.key);
    }
  }
  printer(" ");
  *current_col += 1;
}

// Prints a string broken up into chunks such that it will never surpass
// LINE_LENGTH columns. It always indents the next line and always
// ends with printing a newline.
// The string will only be printed up to the pointer up_to.
// If up_to is NULL then it won't be taken into account.
static void print_up_to(const char *string, const char *up_to,
                        size_t current_col, size_t indent,
                        const tiny_argp_printer_t printer) {
  // NULL string special case just prints a newline and exits
  if (*string == '\0') {
    printer("\r\n");
    return;
  }

  // If string would surpass up_to then print up to a whitespace and
  // newline + indent
  while (*string) {
    // reserve last column for newline char
    if (up_to != NULL && (current_col + up_to - string) < (LINE_LENGTH - 1)) {
      printer("%.*s\r\n", up_to - string, string);
      break;
    } else if ((current_col + strlen(string)) < (LINE_LENGTH - 1)) {
      printer("%s\r\n", string);
      break;
    }

    // Check for newlines
    const char *eol = string + (LINE_LENGTH - current_col);
    while (eol > string && *eol != '\n') {
      eol--;
    }
    if (eol != string) {
      printer("%.*s", eol + 1 - string, string);
      string = eol + 1;
    } else {
      // Look for whitespace to break apart
      const char *break_point = string + (LINE_LENGTH - current_col);
      while (break_point > string && !isspace((unsigned char)*break_point)) {
        break_point--;
      }

      // If pointer went all the way back to start try to find whitespace
      // forward
      if (break_point == string) {
        break_point = string + (LINE_LENGTH - current_col);
        while (*break_point && !isspace((unsigned char)*break_point)) {
          break_point++;
        }
      }

      printer("%.*s\r\n", break_point - string, string);
      string = break_point;
    }

    if (*string && *string == ' ') {
      string++;
    }
    if (*string) {
      printer("%*s", indent, "");
      current_col = indent;
    }
  }
}

static void print_usage(const struct tiny_argp *tiny_argp,
                        const tiny_argp_printer_t printer, unsigned flags,
                        const char *name, bool include_help) {
  printer("Usage: ", name);
  size_t current_col = strlen("Usage:  ");
  if (name != NULL) {
    printer("%s ", name);
    current_col += strlen(name);
    current_col %= LINE_LENGTH;
  }

  if (flags & TINY_ARGP_HELP_SHORT_USAGE) {
    const char *short_usage_option = "[OPTION...] ";
    usage_newline_if_eol(&current_col, strlen(short_usage_option), printer);
    current_col += strlen(short_usage_option);
    printer(short_usage_option);

  } else {
    // Print the short options that don't take in args in a single pair of
    // brackets
    size_t i = 0;
    size_t noarg_shorts = 0;
    while (!is_end_option(tiny_argp->options[i])) {
      struct tiny_argp_option option = tiny_argp->options[i];
      i++;
      if (isprint(option.key) && option.arg == 0) {
        if (!noarg_shorts) {
          printer("[");
        }
        printer("%c", option.key);
        noarg_shorts++;
      }
    }

    if (include_help) {
      if (!noarg_shorts) {
        printer("[");
      }
      printer("%c", help_options[0].key);
      noarg_shorts++;
    }

    if (noarg_shorts) {
      printer("] ");
      current_col += 3 + noarg_shorts;
    }

    for (enum arg_dashes dashes = SHORT; dashes <= LONG; dashes++) {
      i = 0;
      while (!is_end_option(tiny_argp->options[i])) {
        struct tiny_argp_option option = tiny_argp->options[i];
        i++;
        print_opt_usage(dashes, option, &current_col, printer);
      }

      if (!include_help) {
        continue;
      }
      i = 0;
      while (!is_end_option(help_options[i])) {
        struct tiny_argp_option option = help_options[i];
        i++;
        print_opt_usage(dashes, option, &current_col, printer);
      }
    }
  }

  usage_newline_if_eol(&current_col, strlen(tiny_argp->args_doc), printer);
  print_up_to(tiny_argp->args_doc, NULL, current_col, USAGE_INDENT, printer);
}

#define HELP_OPT_DOC_INDENT 29
#define HELP_MAX_OPT_DOC_INDENT 35
static void print_opt_help(const struct tiny_argp_option opt,
                           const tiny_argp_printer_t printer) {
  // Early exit conditions
  if (opt.flags & OPTION_HIDDEN) {
    return;
  }
  bool print_short = isprint(opt.key);
  bool print_long = opt.name != 0;
  if (!print_short && !print_long) {
    return;
  }

  size_t current_col = 0;

  if (print_short) {
    printer("  -%c", (char)opt.key);
    current_col += 4;
  }

  if (print_long) {
    if (print_short) {
      printer(", ");
      current_col += 2;
    } else {
      const char *long_only_indent = "      ";
      printer(long_only_indent);
      current_col += strlen(long_only_indent);
    }
    printer("--%s", opt.name);
    current_col += 2 + strlen(opt.name);
  }

  if (opt.arg != NULL) {
    if (opt.flags & OPTION_ARG_OPTIONAL) {
      if (print_long) {
        printer("[=%s]", opt.arg);
        current_col += 3 + strlen(opt.arg);
      } else {
        printer("[%s]", opt.arg);
        current_col += 2 + strlen(opt.arg);
      }
    } else {
      if (print_long) {
        printer("=%s", opt.arg);
      } else {
        printer(" %s", opt.arg);
      }
      current_col += 1 + strlen(opt.arg);
    }
  }

  // Indentation before doc string
  if (current_col >= HELP_MAX_OPT_DOC_INDENT - 3) {
    printer("\r\n%*s", HELP_OPT_DOC_INDENT, "");
    current_col = HELP_OPT_DOC_INDENT;
  } else if (current_col >= HELP_OPT_DOC_INDENT - 1) {
    printer("   ");
    current_col += 3;
  } else {
    printer("%*s", HELP_OPT_DOC_INDENT - current_col, "");
    current_col = HELP_OPT_DOC_INDENT;
  }

  print_up_to(opt.doc, NULL, current_col, HELP_OPT_DOC_INDENT, printer);
}

static void print_long_help(const struct tiny_argp_option *options,
                            const tiny_argp_printer_t printer,
                            bool include_help) {
  bool print_arg_info = false;
  while (!is_end_option(*options)) {
    print_opt_help(*options, printer);
    if (options->name != 0 && isprint(options->key) && options->arg != 0) {
      print_arg_info = true;
    }
    options++;
  }

  if (include_help) {
    options = help_options;
    while (!is_end_option(*options)) {
      print_opt_help(*options, printer);
      options++;
    }
  }

  // Newline in the middle is to keep it under 80 chars
  if (print_arg_info) {
    printer(
        "\r\nMandatory or optional arguments to long options are also "
        "mandatory "
        "or optional\r\nfor any corresponding short options.\r\n");
  }
}

// The general function that prints help messages, called by API help functions
// directly
static void help(const struct tiny_argp *tiny_argp, const tiny_argp_printer_t printer,
                 unsigned flags, char *name, bool include_help) {
  bool printed = false;
  if (flags & (TINY_ARGP_HELP_USAGE | TINY_ARGP_HELP_SHORT_USAGE)) {
    print_usage(tiny_argp, printer, flags, name, include_help);
    printed = true;
  }

  const char *vertical_tab = tiny_argp->doc;
  while (*vertical_tab && (*vertical_tab != '\v')) {
    vertical_tab++;
  }

  if (flags & TINY_ARGP_HELP_PRE_DOC) {
    print_up_to(tiny_argp->doc, vertical_tab, 0, 0, printer);
    printed = true;
  }

  if (flags & TINY_ARGP_HELP_SEE) {
    printer("Try `%s --help' or `%s --usage' for more information.\r\n", name,
            name);
    printed = true;
  }

  if (flags & TINY_ARGP_HELP_LONG) {
    if (printed) {
      printer("\r\n");
    }
    printed = true;
    print_long_help(tiny_argp->options, printer, include_help);
  }

  if (flags & TINY_ARGP_HELP_POST_DOC) {
    if (*vertical_tab == '\v') {
      if (printed) {
        printer("\r\n");
      }
      print_up_to(vertical_tab + 1, NULL, 0, 0, printer);
    }
    printed = true;
  }
}

/*****************************************************************************
 * Control flow function definitions
 ****************************************************************************/
// Check to determine if the given option is the last of an option array
bool is_end_option(const struct tiny_argp_option option) {
  return option.key == 0 && option.name == 0;
}

// Checks if the given "key" is one of the user provided options
static bool is_user_option(int key, const struct tiny_argp_option *options) {
  while (!is_end_option(*options)) {
    if (options->key == key) {
      return options;
    }

    options++;
  }

  return NULL;
}

static bool is_help_option(const struct tiny_argp_option *to_check) {
  const struct tiny_argp_option *opt = help_options;
  while (!is_end_option(*opt)) {
    if (opt == to_check) {
      return true;
    }
    opt++;
  }
  return false;
}

// The parser function shouldn't be called directly, instead it should be
// called through this wrapper which sets the necessary state
enum exit_type call_parser(int key, char *arg, struct tiny_argp_state *state) {
  size_t previous_next = state->next;

  int res = state->root_tiny_argp->parser(key, arg, state);

  // Manual exit happens when tiny_argp_exit() is called
  if (PSTATE(state)->manual_exit) {
    state->arg_num = PSTATE(state)->arg_num;  // Overwrite public arg_num
    return EXIT_STATE(state);
  }

  // If there's an error different from TINY_ARGP_ERR_UNKNOWN
  else if (res != TINY_ARGP_SUCCESS && res != TINY_ARGP_ERR_UNKNOWN) {
    if ((state->flags & TINY_ARGP_NO_EXIT) && (key != TINY_ARGP_KEY_INIT)) {
      EXIT_STATE(state) = ERROR_THEN_EXIT;
    } else {
      EXIT_STATE(state) = IMMEDIATE_EXIT;
    }
    EXIT_CODE(state) = res;
  }

  // When ERR_UNKNOWN or SUCCESS was returned for non-option arg
  else if (key == TINY_ARGP_KEY_ARG) {
    if (res == TINY_ARGP_ERR_UNKNOWN) {
      state->next--;
      call_parser(TINY_ARGP_KEY_ARGS, NULL, state);
    }
    // Necessarily res is SUCCESS inside else clause
    // (If next is rewinded option isn't considered processed)
    else if (state->next >= previous_next) {
      PSTATE(state)->no_args = false;
      PSTATE(state)->arg_num += (1 + state->next - previous_next);
    }
  }

  // Special KEY_ARGS handling
  else if (key == TINY_ARGP_KEY_ARGS) {
    EXIT_CODE(state) = TINY_ARGP_SUCCESS;
    EXIT_STATE(state) = GOTO_END;

    if (res == TINY_ARGP_SUCCESS) {
      PSTATE(state)->no_args = false;
      // Consider all remaining args to have been consumed
      if (previous_next == state->next) {
        state->next = state->argc;
        PSTATE(state)->arg_num += (state->next - previous_next);
      }
    }
  }

  // If UNKNOWN is returned on a user-defined option
  else if (res == TINY_ARGP_ERR_UNKNOWN &&
           is_user_option(key, state->root_tiny_argp->options)) {
    // Only case when ERR_UNKNOWN is considered a "real" error is
    // when it's returned from a user-defined key.
    opt_error(
        "%s: %s: (PROGRAM ERROR) Option should have been "
        "recognized!?\r\n",
        EINVAL, PSTATE(state)->current_opt_name, state);
  }

  state->arg_num = PSTATE(state)->arg_num;  // Overwrite public arg_num
  return EXIT_STATE(state);
}

// Checks whether the given opt string corresponds to the given option struct.
// opt string may have the argument attached in the same string.
static bool check_option(const char *opt, const enum arg_dashes dashes,
                         const struct tiny_argp_option option) {
  switch (dashes) {
    case SHORT:
      if (!isprint(option.key)) {
        break;
      }
      if (!strncmp(opt, (char *)&option.key, 1)) {
        return true;
      }
      break;
    case LONG:;
      if (option.name == 0) {
        break;
      }
      size_t name_len = strlen(option.name);
      size_t opt_len = strlen(opt);
      if (opt_len < name_len) {
        break;
      }
      if (strncmp(opt, option.name, name_len)) {
        break;
      }
      if (opt_len == name_len) {
        return true;
      }

      // strlen(opt) > option.name implied
      // '=' can separate argument
      if (opt[name_len] == '=') {
        return true;
      }

      break;
  }
  return false;
}

// Calls check_option to see if the given option is one of the user-defined
// options, if it is the _option struct is returned. If it isn't one of
// them NULL is returned.
static const struct tiny_argp_option *find_option(
    const char *opt, const enum arg_dashes dashes,
    const struct tiny_argp_option *options) {
  while (!is_end_option(*options)) {
    if (check_option(opt, dashes, *options)) {
      return options;
    }

    options++;
  }

  return NULL;
}

// Retrieves the possible argument of an opt. It takes into account the
// different shell semantics possible. May advance the state->next value.
static char *get_arg(struct opt_arg *opt_arg, struct tiny_argp_state *state) {
  if (state->next >= state->argc) {
    return NULL;
  }

  bool takes_arg = opt_arg->option->arg != NULL;
  bool requires_arg = (opt_arg->option->flags & OPTION_ARG_OPTIONAL) == 0;
  char *arg;

  switch (opt_arg->size) {
    case SHORT:
      if (!takes_arg) {
        return NULL;
      }
      if (opt_arg->opt[1] != '\0') {
        return opt_arg->opt + 1;
      }
      if (!requires_arg) {
        return NULL;
      }
      state->next++;
      if (state->next >= state->argc) {
        return NULL;
      }
      arg = state->argv[state->next];
      return arg;
    case LONG:;
      // should be safe because option should've been
      // validated before calling this function
      size_t len = strlen(opt_arg->option->name);
      if (opt_arg->opt[len] == '=') {
        return opt_arg->opt + len + 1;
      }
      if (!takes_arg || !requires_arg) {
        return NULL;
      }
      state->next++;
      if (state->next >= state->argc) {
        return NULL;
      }
      arg = state->argv[state->next];
      return arg;
  }

  return NULL;
}

// Gets the full opt_arg object for an opt string. If .option field is NULL
// then the given opt string doesn't pertain to any of the user-defined
// options.
static struct opt_arg get_opt_arg(char *opt, enum arg_dashes dashes,
                                  struct tiny_argp_state *state) {
  const struct tiny_argp_option *option =
      find_option(opt, dashes, state->root_tiny_argp->options);
  if (option == NULL && !(state->flags & TINY_ARGP_NO_HELP)) {
    option = find_option(opt, dashes, help_options);
  }

  struct opt_arg opt_arg = {
      .opt = opt, .size = dashes, .option = option, .arg = NULL};

  if (opt_arg.option == NULL) {
    return opt_arg;
  }

  opt_arg.arg = get_arg(&opt_arg, state);

  return opt_arg;
}

// The boolean check necessary to tell if there's more short options left to
// be parsed in the opt of the given opt_arg. For example if you have the
// short options 'q', and 'v', and let's say 'v' takes an argument. Then the
// option
// '-qvfoo' should be interpreted as both flags 'q' and 'v' with 'foo' as v's
// argument.
static bool check_more_short_options(const struct opt_arg *opt_arg) {
  return opt_arg->size == SHORT && opt_arg->arg == NULL &&
         opt_arg->opt[1] != '\0';
}

// Returns which type of option opt is (short or long) and advances the
// pointer beyond the dashes.
static enum arg_dashes get_length_and_advance(char **opt) {
  enum arg_dashes dashes;
  if (*((*opt) + 1) != '-') {
    *opt += 1;
    dashes = SHORT;
  } else {
    *opt += 2;
    dashes = LONG;
  }
  return dashes;
}

// Parses a single option 'opt'. Should be called by process_options and
// process_any.
static enum exit_type process_option(char *opt, struct tiny_argp_state *state) {
  PSTATE(state)->current_opt_name = opt;
  enum arg_dashes dashes = get_length_and_advance(&opt);
  bool more_short_options;

  do {
    struct opt_arg opt_arg = get_opt_arg(opt, dashes, state);

    if (opt_arg.option == NULL) {
      if (opt_arg.size == SHORT) {
        // There might be more short opts following it in opt string
        const char opt_name[] = {*opt, '\0'};
        opt_error("%s: invalid option '%s'\r\n", EINVAL, &opt_name[0], state);
      } else {
        opt_error("%s: unrecognized option '%s'\r\n", EINVAL,
                  PSTATE(state)->current_opt_name, state);
      }
    } else if (opt_arg.option->arg != 0 && opt_arg.arg == NULL &&
               !(opt_arg.option->flags & OPTION_ARG_OPTIONAL)) {
      if (opt_arg.size == SHORT) {
        // No need to create opt_name because it will always be the last short
        // option in the string
        opt_error("%s: option requires an argument -- '%s'\r\n ", EINVAL, opt,
                  state);
      } else {
        opt_error("%s: option '%s' requires an argument\r\n", EINVAL,
                  PSTATE(state)->current_opt_name, state);
      }
    } else if (opt_arg.option->arg == NULL && opt_arg.arg != NULL) {
      opt_error("%s: option '%s' doesn't allow an argument\r\n", EINVAL,
                PSTATE(state)->current_opt_name, state);
    }

    if (EXIT_STATE(state)) {
      return EXIT_STATE(state);
    }

    more_short_options = check_more_short_options(&opt_arg);

    if (!more_short_options) {
      state->next++;
    }

    if (is_help_option(opt_arg.option)) {
      switch (opt_arg.option->key) {
        case '?':
          tiny_argp_state_help(state, state->root_tiny_argp->printer,
                             TINY_ARGP_HELP_STD_HELP);
          break;
        default:
          // Only other option is --usage
          tiny_argp_state_help(state, state->root_tiny_argp->printer,
                             TINY_ARGP_HELP_USAGE);
      }
      // Immediate exit on help options
      if (!(state->flags & TINY_ARGP_NO_EXIT)) {
        EXIT_STATE(state) = IMMEDIATE_EXIT;
        EXIT_CODE(state) = TINY_ARGP_HELP_OPT;
      }
    } else {
      call_parser(opt_arg.option->key, opt_arg.arg, state);
    }

    if (EXIT_STATE(state)) {
      return EXIT_STATE(state);
    }

    if (more_short_options) {
      opt++;
    }

  } while (more_short_options);

  return TINY_ARGP_SUCCESS;
}

// Starting at state->next starts parsing the options in argv. Skipping
// non-option arguments.
static enum exit_type process_options(struct tiny_argp_state *state) {
  while (state->next < state->argc) {
    char *opt = state->argv[state->next];

    if (*opt != '-') {
      state->next++;
      continue;
    }

    if (!strcmp(opt, "--")) {
      state->next++;
      break;
    }

    if (process_option(opt, state)) {
      return EXIT_STATE(state);
    }
  }

  return TINY_ARGP_SUCCESS;
}

// Shifts a pointer inside argv that's at the index indicated by "origin" into
// the index indicated by "destination". Shifting all the other pointers by
// one to make space.
static void shift_arg(size_t origin, size_t destination, char **argv) {
  if (origin == destination) {
    return;
  }

  char *shifted_arg = argv[origin];

  if (origin < destination) {
    for (size_t i = origin; i < destination; i++) {
      argv[i] = argv[i + 1];
    }
  } else {  // origin > destination
    for (size_t i = origin; i > destination; i--) {
      argv[i] = argv[i - 1];
    }
  }

  argv[destination] = shifted_arg;
}

// Rearranges argv to put all the options first followed by all the non-option
// arguments.
static void shift_opts_back(struct tiny_argp_state *state) {
  // This should be done after process_options,
  // which is why no error checking is done

  if (state->flags & TINY_ARGP_PARSE_ARGV0) {
    state->next = 0;
  } else {
    state->next = 1;
  }
  size_t next_opt_space = state->next;

  while (state->next < state->argc) {
    char *opt = state->argv[state->next];

    if (*opt != '-') {
      state->next++;
      continue;
    }

    if (!strcmp(opt, "--")) {
      shift_arg(state->next, next_opt_space, state->argv);
      PSTATE(state)->quote_index = next_opt_space;
      state->quoted = PSTATE(state)->quote_index + 1;
      next_opt_space++;
      state->next++;
      break;
    }

    enum arg_dashes dashes = get_length_and_advance(&opt);
    bool more_short_options;

    do {
      struct opt_arg opt_arg = get_opt_arg(opt, dashes, state);
      more_short_options = check_more_short_options(&opt_arg);

      if (more_short_options) {
        opt++;
      } else {
        shift_arg(state->next, next_opt_space, state->argv);
        next_opt_space++;
        state->next++;

        // check for disjointed arg
        bool has_arg = opt_arg.arg != NULL;
        bool short_exact = opt_arg.size == SHORT && opt[1] == '\0';
        bool long_exact =
            opt_arg.size == LONG && opt[strlen(opt_arg.option->name)] == '\0';

        bool has_disjointed_arg = has_arg && (short_exact || long_exact);

        if (has_disjointed_arg) {
          // get_opt_arg advanced the state->next pointer,
          // because of this the arg was already moved but the
          // option remains.
          shift_arg(state->next - 1, next_opt_space - 1, state->argv);
          next_opt_space++;
          state->next++;
        }
      }

    } while (more_short_options);
  }

  state->next = next_opt_space;
}

// Processes all the arguments left in argv, both options and non-options are
// parsed.
static enum exit_type process_any(struct tiny_argp_state *state) {
  while (state->next < state->argc) {
    char *arg = state->argv[state->next];

    if (!strcmp(arg, "--") && state->next <= PSTATE(state)->quote_index) {
      PSTATE(state)->quote_index = state->next;
      state->quoted = PSTATE(state)->quote_index + 1;
      state->next++;
      continue;
    }

    bool is_arg = (*arg != '-') || (state->next > PSTATE(state)->quote_index);
    if (is_arg) {
      state->next++;
      if (!(state->flags & TINY_ARGP_NO_ARGS)) {
        if (call_parser(TINY_ARGP_KEY_ARG, arg, state)) {
          return EXIT_STATE(state);
        }
      }
      continue;
    }

    else {
      if (process_option(arg, state)) {
        return EXIT_STATE(state);
      }
    }
  }

  return TINY_ARGP_SUCCESS;
}

// All the checks necessary to determine if all non-option args were consumed
// (for calling END)
static bool check_consumed_all_args(const struct tiny_argp_state *state) {
  if (state->next == state->argc) {
    return true;
  }

  size_t next = state->next;  // Copy to not alter state
  while (next < state->argc) {
    if (next == PSTATE(state)->quote_index) {
      next++;
      continue;
    }

    char *arg = state->argv[next];

    bool is_arg = (*arg != '-') || (next > PSTATE(state)->quote_index);
    if (is_arg) {
      return false;
    }

    else {
      next++;
    }
  }

  return true;
}

/*****************************************************************************
 * API parser function definition
 ****************************************************************************/
int tiny_argp_parse(const struct tiny_argp *tiny_argp, size_t argc, char **argv,
                  unsigned flags, size_t *arg_index, void *input) {
  struct pstate private_state = {
      .force_exit = NO_EXIT,
      .exit_code = 0,
      .no_args = true,
      .current_opt_name = NULL,
      .arg_num = 0,
      .quote_index = argc,
      .manual_exit = false,
  };
  struct tiny_argp_state state_obj = {.root_tiny_argp = tiny_argp,
                                    .argc = argc,
                                    .argv = argv,
                                    .next = 0,
                                    .flags = flags,
                                    .arg_num = 0,
                                    .quoted = 0,
                                    .input = input,
                                    .name = NULL,
                                    .pstate = (void *)&private_state};
  struct tiny_argp_state *state = &state_obj;

  if (call_parser(TINY_ARGP_KEY_INIT, NULL, state)) {
    return EXIT_CODE(state);
  }

  // When passing ARGV0 user should overwrite the name field.
  if (!(flags & TINY_ARGP_PARSE_ARGV0)) {
    state->name = *argv;
    state->next++;
  }

  bool skip_to_exit_check = false;

  if (!(flags & TINY_ARGP_IN_ORDER)) {
    //  First options
    if (process_options(state)) {
      skip_to_exit_check = true;
    }

    // Then shift options back
    if (!skip_to_exit_check) shift_opts_back(state);
  }

  // Then args / all
  if (!skip_to_exit_check) process_any(state);

  // Exit check
  switch (EXIT_STATE(state)) {
    case IMMEDIATE_EXIT:
      return EXIT_CODE(state);
    case ERROR_THEN_EXIT:
      break;
    case GOTO_END:
    case NO_EXIT:
    default:;
      // Check for NO_ARGS
      if (PSTATE(state)->no_args) {
        call_parser(TINY_ARGP_KEY_NO_ARGS, NULL, state);
      }

      if (check_consumed_all_args(state)) {
        call_parser(TINY_ARGP_KEY_END, NULL, state);
      }

      if (arg_index != NULL) {
        *arg_index = state->next;
      }
  }

  if (EXIT_CODE(state) != TINY_ARGP_SUCCESS &&
      EXIT_CODE(state) != TINY_ARGP_ERR_UNKNOWN) {
    call_parser(TINY_ARGP_KEY_ERROR, NULL, state);
  } else {
    call_parser(TINY_ARGP_KEY_SUCCESS, NULL, state);
  }

  // Only instance of calling parser directly, return is simply ignored for
  // FINI
  state->root_tiny_argp->parser(TINY_ARGP_KEY_FINI, NULL, state);
  return EXIT_CODE(state);
}

/*****************************************************************************
 * API *help* function definitions
 ****************************************************************************/
void tiny_argp_help(const struct tiny_argp *tiny_argp,
                  const tiny_argp_printer_t printer, unsigned flags, char *name) {
  help(tiny_argp, printer, flags, name, false);
}

void tiny_argp_state_help(const struct tiny_argp_state *state,
                        tiny_argp_printer_t printer, unsigned flags) {
  if (state->flags & TINY_ARGP_NO_ERRS) {
    return;
  }

  bool include_help = !(state->flags & TINY_ARGP_NO_HELP);
  help(state->root_tiny_argp, printer, flags, state->name, include_help);
}

void tiny_argp_usage(const struct tiny_argp_state *state) {
  tiny_argp_state_help(state, state->root_tiny_argp->err_printer,
                     TINY_ARGP_HELP_STD_USAGE);
}

void tiny_argp_error(const struct tiny_argp_state *state, const char *error) {
  state->root_tiny_argp->err_printer("%s: %s\r\n", state->name, error);
  tiny_argp_state_help(state, state->root_tiny_argp->err_printer,
                     TINY_ARGP_HELP_STD_ERR);
}

void tiny_argp_exit(const struct tiny_argp_state *state, int status) {
  PSTATE(state)->manual_exit = true;
  EXIT_STATE(state) = IMMEDIATE_EXIT;
  EXIT_CODE(state) = status;
}
