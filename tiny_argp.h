#ifndef TINY_ARGP_H
#define TINY_ARGP_H

#include <stddef.h>

/*****************************************************************************
 * Error codes
 ****************************************************************************/
#define TINY_ARGP_SUCCESS 0
/* What to return for unrecognized keys (UNKNOWN).  For special TINY_ARGP_KEY_
 * keys, such returns will simply be ignored.  For user keys, this error will
 * be turned into TINY_ARGP_ERR_PARSE; returning TINY_ARGP_ERR_PARSE itself
 * would result in an immediate stop to parsing in *all* cases.  */
#define TINY_ARGP_ERR_UNKNOWN (-51)
/* Returned if tiny_argp exits early because a '--help' or '--usage' option was
 * parsed. */
#define TINY_ARGP_HELP_OPT (-52)
/* Returned by tiny_argp_parse when the library itself rejects the command line
 * (e.g. unknown option, missing required argument, malformed input). */
#define TINY_ARGP_ERR_PARSE (-53)

// # Early exit behaviour:
//
// ## Possible errors
//
// During option parsing:
//	- <name>: unrecognized option 'opt_given_with_dashes_and_arg_maybe'
//	- <name>: option '--opt_given' requires an argument
//	- <name>: option '--long_opt' doesn't allow an argument
//	- <name>: option requires an argument -- 'opt_given'
//
// Return UNKNOWN on known key:
//	- <name>: <opt_given_with_dashes>: (PROGRAM ERROR) Option should have
// been recognized!?
//
// Return UNKNOWN on TINY_ARGP_KEY_ARGS:
//	- Parses SUCESS key then FINI (always)
//	- On SUCCESS still skips ahead to END
//
// ## NO_EXIT:
//  Instant exit on the following:
//		- INIT parser error
//
//	Passes ERROR key next then FINI:
//		- Parser returns Non-UNKNOWN error
//		- option parsing errors
//		- Return UNKNOWN on known key
//
//	Continues as if nothing:
//	  - --help/--usage
//
//
// ## !NO_EXIT:
//
// Instant exit on the following:
//	- INIT parser error
//	- option parsing errors
//	- Return UNKNOWN on known key
//	- --help/--usage
//
// Passes ERROR key next then FINI:
//	- Parser returns Non-UNKNOWN error

/*****************************************************************************
 * Command options
 ****************************************************************************/
/* A description of a particular option.  A pointer to an array of
   these is passed in the OPTIONS field of an tiny_argp structure. This should
   be terminated by an entry with zero in all fields. Note that when using
   an initialized C array for options, writing { 0 } is enough to achieve
   this. Each option entry can correspond to one long option and/or one
   short option.
 */
struct tiny_argp_option {
  /* The long name for this option, corresponding to the long option ‘--name’;
     this field may be zero if this option only has a short name.  */
  const char *name;

  /* What key is returned for this option.  If > 0 and printable, then it's
     also accepted as a short option.  */
  int key;

  /* Name of the argument associated with this option, shown in help output
     (e.g. "--foo=NAME"). If NULL, the option takes no argument (boolean flag).
     If non-NULL, an argument is required unless OPTION_ARG_OPTIONAL is set. */
  const char *arg;

  /* OPTION_ flags.  */
  unsigned flags;

  /* The doc string for this option.	*/
  const char *doc;
};

/* The argument associated with this option is optional.  */
#define OPTION_ARG_OPTIONAL 0x1

/* This option isn't displayed in any help messages.  */
#define OPTION_HIDDEN 0x2

struct tiny_argp_state; // fwd declare this type
typedef int (*tiny_argp_parser_t)(int key, char *arg,
                                  struct tiny_argp_state *state);
typedef int (*tiny_argp_printer_t)(const char *fmt, ...);

/*****************************************************************************
 * Special Keys
 ****************************************************************************/
/* Special values for the KEY argument to an argument parsing function.
   TINY_ARGP_ERR_UNKNOWN should be returned if they aren't understood.

   The sequence of keys to a parsing function is either (where each
   uppercased word should be prefixed by `TINY_ARGP_KEY_' and opt is a user
   key):

       INIT opt... NO_ARGS END SUCCESS  -- No non-option arguments at all
   or  INIT (opt | ARG)... END SUCCESS  -- All non-option args parsed
   or  INIT (opt | ARG)... SUCCESS      -- Some non-option arg unrecognized

   The third case is where the parser returned TINY_ARGP_ERR_UNKNOWN for an
   argument, in which case parsing stops at that argument (returning the
   unparsed arguments to the caller of tiny_argp_parse if requested, or stopping
   with an error message if not).

   If an error occurs (either detected by tiny_argp, or because the parsing
   function returned an error value), then the parser is called with
   TINY_ARGP_KEY_ERROR, and no further calls are made.  */

/* This is not an option at all, but rather a command line argument.  If a
   parser receiving this key returns success, the fact is recorded, and the
   TINY_ARGP_KEY_NO_ARGS case won't be used. */
#define TINY_ARGP_KEY_ARG 0
/* There are remaining arguments not parsed by the parser, which may be found
   starting at (STATE->argv + STATE->next).  If success is returned, but
   STATE->next left untouched, it's assumed that all arguments were consume,
   otherwise, the parser should adjust STATE->next to reflect any arguments
   consumed.  */
#define TINY_ARGP_KEY_ARGS 0x1000006
/* There are no more command line arguments at all.  */
#define TINY_ARGP_KEY_END 0x1000001
/* Because it's common to want to do some special processing if there aren't
   any non-option args, user parsers are called with this key if they didn't
   successfully process any non-option arguments.  Called just before
   TINY_ARGP_KEY_END (where more general validity checks on previously parsed
   arguments can take place).  */
#define TINY_ARGP_KEY_NO_ARGS 0x1000002
/* Passed in before any parsing is done.  */
#define TINY_ARGP_KEY_INIT 0x1000003
/* Use after all other keys, including SUCCESS & END.  */
#define TINY_ARGP_KEY_FINI 0x1000007
/* Passed in when parsing has successfully been completed (even if there are
   still arguments remaining).  */
#define TINY_ARGP_KEY_SUCCESS 0x1000004
/* Passed in if an error occurs.  */
#define TINY_ARGP_KEY_ERROR 0x1000005

/*****************************************************************************
 * Parser
 ****************************************************************************/
/* An tiny_argp structure contains a set of options declarations, a function to
   deal with parsing one, documentation string. */
struct tiny_argp {
  /* An array of tiny_argp_option structures, terminated by an entry with both
     NAME and KEY having a value of 0.  */
  const struct tiny_argp_option *options;

  /* What to do with an option from this structure.  KEY is the key
     associated with the option, and ARG is any associated argument (NULL if
     none was supplied).  If KEY isn't understood, TINY_ARGP_ERR_UNKNOWN should
     be returned.  If a non-zero, non-TINY_ARGP_ERR_UNKNOWN value is returned,
     then parsing is stopped immediately, and that value is returned from
     tiny_argp_parse().  For special (non-user-supplied) values of KEY, see the
     TINY_ARGP_KEY_ definitions below.  */
  tiny_argp_parser_t parser;

  /* A string describing what other arguments are wanted by this program.  It
     is only used by tiny_argp_usage to print the `Usage:' message.  */
  const char *args_doc;

  /* If non-NULL, a string containing extra text to be printed before and
     after the options in a long help message (separated by a vertical tab
     `\v' character).  */
  const char *doc;

  const tiny_argp_printer_t printer;
  const tiny_argp_printer_t err_printer;
};

/* Parsing state.  This is provided to parsing functions called by tiny_argp,
   which may examine and, as noted, modify fields.  */
struct tiny_argp_state {
  /* The top level TINY_ARGP being parsed.  */
  const struct tiny_argp *root_tiny_argp;

  /* The argument vector being parsed.  May be modified.  */
  size_t argc;
  char **argv;

  /* The index in ARGV of the next arg that to be parsed.  May be modified.
     One way to consume all remaining arguments in the input is to set
     state->next = state->argc. The current option can be re-parsed immediately
     by decrementing this field, then modifying state->argv[state->next] to
     reflect the option that should be reexamined. */
  size_t next;

  /* The flags supplied to tiny_argp_parse.  May be modified.  */
  unsigned flags;

  /* While calling a parsing function with a key of TINY_ARGP_KEY_ARG, this is
     the number of the current arg, starting at zero.  At all other times, this
     is the number of such arguments that have been processed.  */
  unsigned arg_num;

  /* If non-zero, the index in ARGV of the first argument following a special
     `--' argument (which prevents anything following being interpreted as an
     option).  Only set once argument parsing has proceeded past this point. */
  size_t quoted;

  /* An arbitrary pointer passed in from the user.  */
  void *input;

  /* The program's name, used when printing messages.  */
  char *name;

  void *pstate; /* Private, for use by tiny_argp.  */
};
/* Flags for tiny_argp_parse (note that the defaults are those that are
   convenient for program command line parsing): */

/* Don't ignore the first element of ARGV.  Normally the first element of
 * the argument vector is skipped for option parsing purposes, as it
 * corresponds to the program name in a command line.  If this flag is set,
 * the library will not populate state->name from argv[0]; it stays as its
 * default `""`.  Overwrite state->name inside the parse function (typically
 * at KEY_INIT) if you want a meaningful program name to appear in error
 * output. */
#define TINY_ARGP_PARSE_ARGV0 0x01

/* Don't print error messages.  */
#define TINY_ARGP_NO_ERRS 0x02

/* Don't parse any non-option args.  */
#define TINY_ARGP_NO_ARGS 0x04

/* Parse options and arguments in the same order they occur on the command
   line -- normally they're rearranged so that all options come first. */
#define TINY_ARGP_IN_ORDER 0x08

/* Don't provide the standard long option --help or --usage, which causes usage
 * and option help information to be output to stdout, and the parser to
 * abort with OK called. */
#define TINY_ARGP_NO_HELP 0x10

/* Don't exit on errors (they may still result in error messages).  */
#define TINY_ARGP_NO_EXIT 0x20

/* Turns off any message-printing/exiting options.  */
#define TINY_ARGP_SILENT                                                       \
  (TINY_ARGP_NO_EXIT | TINY_ARGP_NO_ERRS | TINY_ARGP_NO_HELP)

/* Parse the options strings in ARGC & ARGV according to the options in
 * TINY_ARGP. FLAGS is one of the TINY_ARGP_ flags above.  If ARG_INDEX is
 * non-NULL, the index in ARGV of the first unparsed arg is returned in it.  If
 * some parser routine returned a non-zero value, it is returned; otherwise 0 is
 * returned.  INPUT is a pointer to a value to be passed in to the parser.  */
int tiny_argp_parse(const struct tiny_argp *tiny_argp, size_t argc, char **argv,
                    unsigned flags, size_t *arg_index, void *input);

/* Flags for tiny_argp_help.  */
#define TINY_ARGP_HELP_USAGE 0x01 /* a Usage: message. */
#define TINY_ARGP_HELP_SHORT_USAGE                                             \
  0x02                               /*  " but don't actually print            \
                                        options.*/
#define TINY_ARGP_HELP_SEE 0x04      /* a `Try ... for more help' message. */
#define TINY_ARGP_HELP_LONG 0x08     /* a long help message. */
#define TINY_ARGP_HELP_PRE_DOC 0x10  /* doc string preceding long help.  */
#define TINY_ARGP_HELP_POST_DOC 0x20 /* doc string following long help.  */
#define TINY_ARGP_HELP_DOC (TINY_ARGP_HELP_PRE_DOC | TINY_ARGP_HELP_POST_DOC)

/* The standard thing to do after a program command line parsing error, if an
   error message has already been printed.  */
#define TINY_ARGP_HELP_STD_ERR (TINY_ARGP_HELP_SEE)
/* The standard thing to do after a program command line parsing error, if no
   more specific error message has been printed.  */
#define TINY_ARGP_HELP_STD_USAGE                                               \
  (TINY_ARGP_HELP_SHORT_USAGE | TINY_ARGP_HELP_SEE)
/* The standard thing to do in response to a --help option.  */
#define TINY_ARGP_HELP_STD_HELP                                                \
  (TINY_ARGP_HELP_SHORT_USAGE | TINY_ARGP_HELP_LONG | TINY_ARGP_HELP_DOC)

/* Output a usage message for TINY_ARGP with PRINTER.  FLAGS are from the set
 * TINY_ARGP_HELP_*. This function won't print the '--help' and --usage options
 */
void tiny_argp_help(const struct tiny_argp *tiny_argp,
                    tiny_argp_printer_t printer, unsigned flags, char *name);

/* The following routines are intended to be called from within a tiny_argp
 * parsing routine (thus taking a tiny_argp_state structure as the first
 * argument).  They may or may not print an error message, depending
 * on the flags in STATE -- in any case, because they *don't* exit, the caller
 * should be prepared for this, and should return an appropriate error
 * after calling them.  [tiny_argp_usage & tiny_argp_error should probably be
 * called tiny_argp_state_..., but they're used often enough that they should be
 * short]
 */
void tiny_argp_state_help(const struct tiny_argp_state *state,
                          tiny_argp_printer_t printer, unsigned flags);

/* Possibly output the standard usage message for TINY_ARGP with err_printer. */
void tiny_argp_usage(const struct tiny_argp_state *state);

/* If appropriate, print the string ERROR, preceded by the program name and `:',
 * with err_printer, and followed by a `Try ... --help' message.  */
void tiny_argp_error(const struct tiny_argp_state *state, const char *error);

/* After calling this function, no matter what the parser function returns, the
 * tiny_argp parser will stop execution and return the given status code.
 * The purpose of this function is to emulate calling the `exit` function, but
 * without fully terminating the runtime. */
void tiny_argp_exit(const struct tiny_argp_state *state, int status);

#endif
