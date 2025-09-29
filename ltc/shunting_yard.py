"""
Converts an infix expression into reverse polish notation
and evalualtes the expression.

The Shunting yard algorithm is based on:
https://www.andreinc.net/2010/10/05/converting-infix-to-rpn-shunting-yard-algorithm
"""

import enum
import collections
import inspect

from . import LP
from . import is_a_valid_name


def pyln() -> int:
    """
    Returns the current line number where this function is called.
    Used for debug print statements
    """
    frame = inspect.currentframe()

    # f_back refers to the caller's frame
    # f_lineno is the line number in that frame
    return frame.f_back.f_lineno


# Defines the available expression operators.
#
Operator = enum.Enum("Operator", ('add', 'sub', 'mult', 'divide'))

# Maps operator char/text to the operator value.
#
Text_To_Operator = {
    "+": Operator.add,
    "-": Operator.sub,
    "*": Operator.mult,
    "/": Operator.divide
}

# Maps operator value to char/text
#
Op_Text = {
    Operator.add: "+",
    Operator.sub: "/",
    Operator.mult: "*",
    Operator.divide: "/"
}

# The following could all "live" within the shunting_yard function.
#
# Operator associativity
#
Associativity = enum.Enum("Associativity", ("LEFT", "RIGHT"))

Operator_Spec = collections.namedtuple("Operator_Spec", ("associativity", "precedence"))


# Monkey patch Operator_Spec equality operators (based on precedence)
#
def lt(this, other):
    return this.precedence < other.precedence


def le(this, other):
    return this.precedence <= other.precedence


Operator_Spec.__lt__ = lt
Operator_Spec.__le__ = le


# List of operator specifications
#
Operator_Specifications = {
    Operator.add: Operator_Spec(Associativity.LEFT, 0),
    Operator.sub: Operator_Spec(Associativity.LEFT, 0),
    Operator.mult: Operator_Spec(Associativity.LEFT, 5),
    Operator.divide: Operator_Spec(Associativity.LEFT, 5)
}


# -----------------------------------------------------------------------------
#
def shunting_yard(tokens: list | tuple) -> list:
    """
    Infix to Reverse Polish notation
    Infix token list must be legal/valid syntax.
    """

    output = []
    stack = []    # operator and ( stack

    for token in tokens:
        if isinstance(token, Operator):
            # Token is an operator
            while len(stack) > 0 and isinstance(stack[-1], Operator):
                cOp = Operator_Specifications[token]         # Current operator
                lOp = Operator_Specifications[stack[-1]]     # Top operator from the stack
                if (cOp.associativity == Associativity.LEFT and cOp <= lOp) or \
                   (cOp.associativity == Associativity.RIGHT and cOp < lOp):
                    # Pop from the stack and add to output buffer
                    #
                    output.append(stack.pop(-1))
                    continue
                break

            # Push the new operator on the stack
            stack.append(token)

        elif token == "(":
            # the token is left parenthesis, then push it on the stack
            stack.append(token)

        elif token == ")":
            # the token is right parenthesis
            while len(stack) > 0 and stack[-1] != "(":
                # Until the top token (from the stack) is left parenthesis,
                # pop from  the stack to the output buffer
                #
                output.append(stack.pop(-1))

            # Also pop the left parenthesis but don't include it in the output
            #
            if len(stack) > 0:
                stack.pop(-1)

        else:
            # Else add token to output buffer
            output.append(token)

    while len(stack) > 0:
        output.append(stack.pop(-1))

    return output


# -----------------------------------------------------------------------------
#
def parse_expression(text: str) -> list[str] | None:
    """
    Parses an infix expression into a list of tokens.
    "+" converted to Operator.add, etc
    Literals of the form =X1234, =C"AB" and 0.6666
    are converted to integers.

    Unaries, i.e. +/-, at start of the expression, or immediately after
    an opening parenthesis, are either dropped if reduntant or converted
    to a binary operation, e.g. ["+", X] => [X],  ["-", X] => [0, "-", X].
    This makes validation and evaluation easier - no unaries to worry
    about.
    """

    parse_expression.message = " message still TBD"

    work = text.split()  # Do an initial split on spaces

    for delim in "+-*/()":
        temp = []
        for item in work:
            parts = item.split(delim)
            for part in parts:
                if part != "":
                    temp.append(part)
                temp.append(delim)

            temp.pop(-1)  # unrequired delim

        work = temp

    intermediate = []
    for term in work:
        if term == ".":
            xterm = LP   # load point / current address

        elif term in Text_To_Operator.keys():
            xterm = Text_To_Operator[term]

        elif term in ('(', ')'):
            # unlike +, - etc. np special enum for parenthsis
            #
            xterm = term

        elif "0" <= term[0] <= "9" and '_' not in term:
            # We might have a decimal or fractional number
            # python allows '_' with numbers, DataCode1 does not.
            #
            try:
                term_value = int(term)
            except ValueError:
                # Not an int - try a float
                # Should we accept .5 or inssist on 0.5
                #
                fvalue = float(term)

                # What should the rounding policy be?
                #
                term_value = int(0x8000 * fvalue)

            # Error or wrap with warning??
            #
            if term_value < -32768 or term_value > +32767:
                raise ValueError(f"{term} exceeds allowed range")

            xterm = term_value

        elif term.startswith("=X"):
            # we have a hexa-decimal number
            #
            subterm = term[2:]
            stl = len(subterm)
            if stl < 1 or stl > 4:
                raise ValueError(f"{term}  len: {stl}")
            term_value = int(subterm, 16)

            if term_value >= 0x8000:
                term_value = term_value - 0x10000
            xterm = term_value

        elif term.startswith("=C"):
            # we have a character number
            # should we do parity?
            #
            subterm = term[2:]
            stl = len(subterm)
            if stl < 3 or stl > 4 or subterm[0] not in ('"', "'") or subterm[0] != subterm[-1]:
                raise ValueError(f"{term}  len: {stl}")

            term_value = 0
            for char in subterm[1:-1]:
                term_value = term_value * 256 + ord(char)

            if term_value >= 0x8000:
                term_value = term_value - 0x10000
            xterm = term_value

        elif is_a_valid_name(term):
            # Is a symbol i.e. name or label
            xterm = term

        else:
            parse_expression.message =f"invalid term {term} ({is_a_valid_name.message})"
            return None

        intermediate.append(xterm)

    # This part does no validation per se, just drops redundant unary '+' and
    # converts  unary '-', X  to  0 '-' X  or to -X where X is an actual value.
    #
    result = []
    can_modify_unary = True
    n = len(intermediate)
    j = 0
    while j < n:
        token = intermediate[j]
        j += 1

        if can_modify_unary:
            if token is Operator.add:
                pass
            elif token is Operator.sub:
                next_token = intermediate[j] if j < n else None
                if isinstance(next_token, int):
                    result.append(-next_token)
                    j += 1
                else:
                    result.append(0)
                    result.append(token)   # i.e. sub
            else:
                result.append(token)

        else:
            result.append(token)

        can_modify_unary = (token == '(')

    return result


# -----------------------------------------------------------------------------
#
Kind = enum.Enum("Kind",
                 ("Start",     # start of expression
                  "BinaryOP",  # +, -, *, /
                  "Term",      #
                  "Open",      # bracket: ()
                  "Close",     # bracket: )
                  "End"))      # end of expression


# We use a white list of allow combinations
# Less items than using a black list.
#
allowed_token_kind_pairs = {
    (Kind.Start, Kind.Term),
    (Kind.Start, Kind.Open),
    (Kind.BinaryOP, Kind.Term),
    (Kind.BinaryOP, Kind.Open),
    (Kind.Term, Kind.BinaryOP),
    (Kind.Term, Kind.Close),
    (Kind.Term, Kind.End),
    (Kind.Open, Kind.Term),
    (Kind.Open, Kind.Open),
    (Kind.Close, Kind.BinaryOP),
    (Kind.Close, Kind.Close),
    (Kind.Close, Kind.End)
}


# -----------------------------------------------------------------------------
#
def validate_infix(tokens: list | tuple) -> bool:
    """
    Performs a sanity check on the infix list.
    Return value is bool indicating okay/invalid.
    The function message attribute provides error info.
    """
    validate_infix.message = ""

    if len(tokens) == 0:
        validate_infix.message = "empty expression"
        return False

    last_kind = Kind.Start
    last_token = "start of expression"
    bracket_depth = 0
    for token in tokens:
        # Determine the tyope of token.
        #
        if isinstance(token, Operator):
            kind = Kind.BinaryOP

        elif token == '(':
            kind = Kind.Open
            bracket_depth += 1

        elif token == ')':
            kind = Kind.Close
            bracket_depth -= 1
            if bracket_depth < 0:
                validate_infix.message = "un-balanced '(' and ')'"
                return False

        elif isinstance(token, (int, str)) or token is LP:
            kind = Kind.Term

        else:
            validate_infix.message = f"unexpected token: {token} : {token.__class__.__name__}"
            return False

        combo = (last_kind, kind)

        if combo not in allowed_token_kind_pairs:
            validate_infix.message = f"'{Op_Text[token]}' cannot immediately follow '{Op_Text[last_token]}'"
            return False

        last_token = token
        last_kind = kind

    if bracket_depth != 0:
        validate_infix.message = "un-balanced '(' and ')'"
        return False

    kind = Kind.End
    combo = (last_kind, kind)

    if combo not in allowed_token_kind_pairs:
        validate_infix.message = f"end of expression cannot immediately follow '{Op_Text[last_token]}'."
        return False

    return True


# -----------------------------------------------------------------------------
#
def calc(v1: int | None, v2: int, op: str) -> int | None:
    if op is Operator.add:
        return v1 + v2
    elif op is Operator.sub:
        return v1 - v2
    elif op is Operator.mult:
        return v1 * v2
    elif op is Operator.divide:
        return v1 // v2 if v2 != 0 else None
    else:
        return None


# -----------------------------------------------------------------------------
#
def evaluate_rpn(rpn_expression: list, values: dict) -> int | None:
    """
    """
    evaluate_rpn.message = ""

    stack = []
    for item in rpn_expression:
        if isinstance(item, Operator):
            # It's an operator'
            v2 = stack.pop()
            v1 = stack.pop()
            vs = calc(v1, v2, item)
            stack.append(vs)

        elif isinstance(item, (int, str)) or item is LP:
            if not isinstance(item, int):
                # Look up symbole value ...
                value = values.get(item, None)
                if value is None:
                    evaluate_rpn.message = f"name '{item}' is not defined"
                    return None
                item = value
            stack.append(item)

        else:
            evaluate_rpn.message = f"{type(item)} is not int or str or '.'"
            return None

    if len(stack) != 1:
        evaluate_rpn.message = f"str(rpn_expression)"
        return None

    return stack.pop()


# -----------------------------------------------------------------------------
#
def evaluate_expression(text: str,
                        address: int,
                        global_values: dict | None,
                        local_values: dict | None) -> int | None:
    """ Evaluate an expression if we can, if not returns the default value.
        Does not do any out of range checking per se.
    """
    evaluate_expression.message = ""

    # First parse the expression
    # converts test to a list of numbers(int), operators(str) or symbols.
    #
    try:
        source = parse_expression(text)
        if source is None:
            evaluate_expression.message = f"parse failed: {parse_expression.message}"
            return None
    except BaseException as e:
        evaluate_expression.message = f"parse failed: {e}"
        return None

    if validate_infix(source) is False:
        evaluate_expression.message = validate_infix.message
        return None

    # Infix sourse has passed sanity check.
    # Convert to reverse polish notation format.
    #
    rpn_expression = shunting_yard(source)

    # Create a simplified dictionary
    #
    values = {}
    if global_values is not None:
        for k, s in global_values.items():
            values[k] = s.value

    if local_values is not None:
        for k, s in local_values.items():
            values[k] = s.value

    values[LP] = address

    result = evaluate_rpn(rpn_expression, values)
    if result is None:
        evaluate_expression.message = evaluate_rpn.message

    return result

# end
