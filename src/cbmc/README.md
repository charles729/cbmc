\ingroup module_hidden
\defgroup cbmc cbmc

# Folder CBMC

\author Martin Brain

This contains the first full application. CBMC is a bounded model
checker that uses the front ends (`ansi-c`, `cpp`, goto-program or
others) to create a goto-program, `goto-symex` to unwind the loops the
given number of times and to produce and equation system and finally
`solvers` to find a counter-example (technically, `goto-symex` is then
used to construct the counter-example trace).

## Architecture

CBMC operates in three phases:

1. Parse command-line options; convert passed source files into a GOTO model
   (for more detail see \ref AST_section).
2. Run `goto-symex` to turn the GOTO model into a SAT or SMT equation
   (for more detail see \ref goto-symex).
3. Solve the equation and report results (described here, but see
   \ref BMC_section for more information).

The GOTO model is generated using `initialize_goto_model`; see documentaton
in the `goto-programs/` directory. Goto-symex is similarly documented in its
own directory.

## Equation solving

The class `bmct` is responsible for solving the equation, generated by
goto-symex, which represents the assertions or goals it is being asked to show
can be hit or satisfied (or prove that can't be done).

For example, given an input program
`f(int x, int y) { assert(x + y % 3 == 1); assert(x % y == 0); }` then goto-
symex would produce a formula something like
`F1 + F2 % 3 != 1 OR (F1 + F2 % 3 == 1 AND F1 % F2 != 0), where F-variables
are free. The precise expression of that formula depends on CBMC's command-line
configuration: for some backends it will lower the arithmetic operations into
bitwise operations; other backends understand arithmetic and so they will remain
as-is. See `cbmc_solverst` for details of how a backend solver is selected,
constructed and configured.

Whatever precise backend representation is used, CBMC controls solving the
formula using two different possible procedures:

1. When `--stop-on-fail` is passed on the command-line, the formula is solved
   once to find any violated assertion. This is implemented directly in
   class `bmct`.
2. When `--stop-on-fail` is not passed, BMC is run in "all-properties" mode.
   This categorises the assertions into groups that represent the same property,
   and the solver is run repeatedly to try to satisfy each property at least
   once. This is implemented in `bmc_all_propertiest`.

For example, in the all-properties case, if the provided formula had five
assertions, `A1 ... A5`, and `A1` and `A2` were instances of the same property
as were `A4` and `A5`, then initially the solver would simply be asked to
satisfy `A1 | A2 | A3 | A4 | A5`, but if its first solution gave an answer for
`A1` only, then the solver would be called again asking for a solution to
`A3 | A4 | A5`, `A1` having been satisfied and `A2` being considered irrelevant
because it is an instance of the same property as `A1`. If the solver came back
with a solution hitting `A3` then it would be asked again to solve for
`A4 | A5`, and so on until either every property is satisfied or the solver
responds that a solution is impossible.

Generally assertions are considered instances of the same property if they are
derived from the same source code statement: for example,
`assert(x == 1); assert(x == 2);` would be considered two distinct properties,
while `for(int i = 1; i <= 2; ++i) assert(x == i);` would generate two instances
of the same property. This is because assertions are categorised by their
`source_location->get_property_id()` field, which in turn is set per static
instruction by `goto-programs/set_properties.cpp`. Assertions that are generated
by other mechanism may categorise their properties differently.

### Coverage mode

There is one further BMC mode that differs more fundamentally: when `--cover` is
passed, assertions in the program text are converted into assumptions (these
must hold for control flow to proceed past them, but are not goals for the
equation solver), while new `assert(false)` statements are added throughout the
source program representing coverage goals. The equation solving process then
proceeds the same as in all-properties mode. Coverage solving is implemented by
`bmc_covert`, but is structurally practically identical to
`bmc_all_propertiest`-- only the reporting differs (goals are called "covered"
rather than "failed").

## Reporting

By default CBMC will report its findings by listing which assertions passed
(could not be violated) or failed (could be violated), or which coverage goals
were satisfiable or unsatisfiable, depending on its mode of operation. However
if `--trace` is passed then it will print a full program trace indicating how
each assertion was violated. This is constructed using `build_goto_trace`, which
queries the backend asking what value was chosen for each program variable on
the path from the start of the program to the relevant assertion. For more
details on how the trace is populated see the documentation for
`build_goto_trace` for `prop_convt::get`, the function used to query the
backend.

## Path-symex

CBMC supports running goto-symex in "path explorer" mode, which alters the
simple GOTO model, goto-symex, equation solving sequence described above by
incrementally running goto-symex to produce a formula describing *part* of a
program, trying to solve that partial formula to satisfy some properties (or
achieve some coverage, as appropriate), before re-entering symex to examine
other properties. For example, given a simple program
`if(x) assert(y); else assert(z);`, standard CBMC would generate a single
formula of the form `(x && !y) || (!x && !z)`, while in path-explorer mode it
might generate a partial formula `x && !y` and try solving that before even
considering the `!x` case. This should have the same end result as without the
special path-explorer mode, but could achieve the result faster in
`--stop-on-fail` mode or allow salvaging a partial result from an otherwise
excessively time- or memory-consuming run in all-properties mode.

For more details on path-symex see the classes `goto_symext` and
`path_explorert`.