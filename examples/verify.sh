#!/usr/bin/env bash
# Verification harness for the tiny_argp examples.
#
# Builds are assumed to already exist under ./build (the `verify` make target
# depends on the binaries).  For each example this script exercises a spread
# of arg combinations and asserts on exit code and stdout+stderr content.
#
# Exit codes reflect what tiny_argp_parse returns via `main`:
#   0     TINY_ARGP_SUCCESS
#   203   TINY_ARGP_ERR_PARSE  (-53 unsigned 8-bit)
#   204   TINY_ARGP_HELP_OPT   (-52 unsigned 8-bit)

set -u
cd "$(dirname "$0")"

BUILD=./build
PASS=0
FAIL=0
FAILED_TESTS=()

if [ -t 1 ]; then
  GREEN=$'\e[32m'; RED=$'\e[31m'; DIM=$'\e[2m'; RESET=$'\e[0m'
else
  GREEN=""; RED=""; DIM=""; RESET=""
fi

# run_check <desc> <expected-exit|any> <expected-substring|""> <cmd...>
run_check() {
  local desc="$1"; shift
  local want_exit="$1"; shift
  local want_match="$1"; shift
  local out rc ok=1
  out=$("$@" 2>&1)
  rc=$?
  if [ "$want_exit" != "any" ] && [ "$rc" -ne "$want_exit" ]; then ok=0; fi
  if [ -n "$want_match" ] && ! grep -qF -- "$want_match" <<< "$out"; then ok=0; fi

  if [ $ok -eq 1 ]; then
    printf "  ${GREEN}PASS${RESET}  %s\n" "$desc"
    PASS=$((PASS+1))
  else
    printf "  ${RED}FAIL${RESET}  %s\n" "$desc"
    printf "        ${DIM}cmd:      %s${RESET}\n" "$*"
    printf "        ${DIM}exit:     got %d, want %s${RESET}\n" "$rc" "$want_exit"
    printf "        ${DIM}expected: %s${RESET}\n" "${want_match:-<none>}"
    printf "        ${DIM}output:${RESET}\n"
    printf '%s\n' "$out" | sed 's/^/          /'
    FAIL=$((FAIL+1))
    FAILED_TESTS+=("$desc")
  fi
}

# ------------------------------------------------------------------
echo "--- ex1_minimal ---"
run_check "no args"                          0   ""       $BUILD/ex1_minimal
run_check "positional args are ignored"      0   ""       $BUILD/ex1_minimal foo bar baz
run_check "--help prints usage"              204 "Usage:" $BUILD/ex1_minimal --help
run_check "-? prints usage (short --help)"   204 "Usage:" $BUILD/ex1_minimal '-?'
run_check "--usage prints short usage"       204 "Usage:" $BUILD/ex1_minimal --usage

# ------------------------------------------------------------------
echo "--- ex2_options ---"
run_check "defaults: verbose=no"             0 "VERBOSE = no"          $BUILD/ex2_options
run_check "defaults: silent=no"              0 "SILENT = no"           $BUILD/ex2_options
run_check "defaults: output=-"               0 "OUTPUT_FILE = -"       $BUILD/ex2_options
run_check "-v enables verbose"               0 "VERBOSE = yes"         $BUILD/ex2_options -v
run_check "--verbose (long form)"            0 "VERBOSE = yes"         $BUILD/ex2_options --verbose
run_check "-q enables silent"                0 "SILENT = yes"          $BUILD/ex2_options -q
run_check "--quiet (long form)"              0 "SILENT = yes"          $BUILD/ex2_options --quiet
run_check "-s (alias for -q)"                0 "SILENT = yes"          $BUILD/ex2_options -s
run_check "--silent (alias for --quiet)"     0 "SILENT = yes"          $BUILD/ex2_options --silent
run_check "-o FILE (space-separated arg)"    0 "OUTPUT_FILE = out.txt" $BUILD/ex2_options -o out.txt
run_check "-oFILE (attached arg)"            0 "OUTPUT_FILE = out.txt" $BUILD/ex2_options -oout.txt
run_check "--output=FILE"                    0 "OUTPUT_FILE = out.txt" $BUILD/ex2_options --output=out.txt
run_check "--output FILE (space form)"       0 "OUTPUT_FILE = out.txt" $BUILD/ex2_options --output out.txt
run_check "bundled -vq: verbose set"         0 "VERBOSE = yes"         $BUILD/ex2_options -vq
run_check "bundled -vq: silent set"          0 "SILENT = yes"          $BUILD/ex2_options -vq
run_check "unknown -x fails"                 203 "invalid option"      $BUILD/ex2_options -x
run_check "unknown --foo fails"              203 "unrecognized option" $BUILD/ex2_options --foo
run_check "-o without arg fails"             203 "requires an argument" $BUILD/ex2_options -o
run_check "--help shows options"             204 "Produce verbose output" $BUILD/ex2_options --help
run_check "-? shows options (short --help)"  204 "Produce verbose output" $BUILD/ex2_options '-?'
run_check "--help shows doc"                 204 "tiny_argp example #2"   $BUILD/ex2_options --help

# ------------------------------------------------------------------
echo "--- ex3_positionals ---"
run_check "no args -> too few"               203 "too few arguments"     $BUILD/ex3_positionals
run_check "one arg captures ARG1"            0   "ARG1 = A"              $BUILD/ex3_positionals A
run_check "one arg: STRINGS empty"           0   "STRINGS ="             $BUILD/ex3_positionals A
run_check "multiple positionals"             0   "STRINGS = B, C"        $BUILD/ex3_positionals A B C
run_check "-o + positionals"                 0   "OUTPUT_FILE = out.txt" $BUILD/ex3_positionals -o out.txt X Y
run_check "opts + positionals: ARG1"         0   "ARG1 = X"              $BUILD/ex3_positionals -v X Y
run_check "opts + positionals: verbose"      0   "VERBOSE = yes"         $BUILD/ex3_positionals -v X Y
run_check "-- separator: --v is positional"  0   "STRINGS = --v"         $BUILD/ex3_positionals A -- --v
# The hidden --silent alias still works even though it's absent from --help.
run_check "hidden --silent still functions"  0   "SILENT = yes"          $BUILD/ex3_positionals --silent A
# ARG1 + 33 strings overflows MAX_STRINGS.
STRINGS_33=$(printf 's%s ' $(seq 1 33))
run_check "overflow: too many strings"       203 "too many string arguments" $BUILD/ex3_positionals A $STRINGS_33
run_check "--help shows args_doc"            204 "ARG1 [STRING...]"      $BUILD/ex3_positionals --help
run_check "-? shows args_doc (short --help)" 204 "ARG1 [STRING...]"      $BUILD/ex3_positionals '-?'
run_check "--help shows pre-doc"             204 "several arguments"     $BUILD/ex3_positionals --help
run_check "--help shows post-doc"            204 "Trailing STRINGs"      $BUILD/ex3_positionals --help
run_check "--help omits hidden --silent"     204 "Produce verbose output" $BUILD/ex3_positionals --help
run_check "--help shows bug-report line"     204 "Report bugs at"        $BUILD/ex3_positionals --help

# ------------------------------------------------------------------
echo "--- ex4_subcommands ---"
run_check "no args -> prints usage"          203 "Usage:"                    $BUILD/ex4_subcommands
run_check "unknown sub-command fails"        203 "unknown sub-command"       $BUILD/ex4_subcommands bogus
run_check "add --name=w"                     0   "adding 1 copies of 'w'"    $BUILD/ex4_subcommands add --name=w
run_check "add --name=w --count=3"           0   "adding 3 copies of 'w'"    $BUILD/ex4_subcommands add --name=w --count=3
run_check "add -n foo (short)"               0   "adding 1 copies of 'foo'"  $BUILD/ex4_subcommands add -n foo
run_check "add -n foo -c 7 (short)"          0   "adding 7 copies of 'foo'"  $BUILD/ex4_subcommands add -n foo -c 7
run_check "add missing --name fails"         203 "--name is required"        $BUILD/ex4_subcommands add
run_check "add error uses compound name"     203 "ex4_subcommands add:"      $BUILD/ex4_subcommands add
run_check "list defaults"                    0   "verbose=no"                $BUILD/ex4_subcommands list
run_check "list -v"                          0   "verbose=yes"               $BUILD/ex4_subcommands list -v
run_check "list --verbose"                   0   "verbose=yes"               $BUILD/ex4_subcommands list --verbose
run_check "sub-command --help works"         204 "Add an item"               $BUILD/ex4_subcommands add --help
run_check "sub-command -? works"             204 "Add an item"               $BUILD/ex4_subcommands add '-?'
run_check "list --help works"                204 "Include hidden items"      $BUILD/ex4_subcommands list --help
run_check "list -? works"                    204 "Include hidden items"      $BUILD/ex4_subcommands list '-?'

# ------------------------------------------------------------------
echo ""
echo "-----------------------------------------"
printf "PASSED: %d\n" "$PASS"
printf "FAILED: %d\n" "$FAIL"
if [ "$FAIL" -gt 0 ]; then
  printf "\nFailed:\n"
  for t in "${FAILED_TESTS[@]}"; do
    printf "  - %s\n" "$t"
  done
  exit 1
fi
exit 0
