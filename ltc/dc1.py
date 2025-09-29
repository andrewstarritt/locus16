"""
DataCode1 assembler.
"""

# SPDX-FileCopyrightText: 2021-2025 Andrew C. Starritt
# SPDX-License-Identifier: GPL-3.0-only

import click
import collections
import datetime
import enum
import os
import pprint
import sys

from . import is_a_valid_name
from .shunting_yard import evaluate_expression


__version__ = "0.4.0"

error_col = "\033[31;1m"
warn_col = "\033[33;1m"
reset_col = "\033[00m"


# -----------------------------------------------------------------------------
# Used for diagnostic purposes
#
try:
    pp = pprint.PrettyPrinter(indent=2, width=180, sort_dicts=False)
except TypeError:
    # Unsorted dictionary option not available in 3.6.8 or earlier.
    #
    pp = pprint.PrettyPrinter(indent=2, width=180)


class DataCodeOneAssesmblerException(Exception):
    pass


# As well as instructions (SETA 42,R and the like) we have:
# empty lines or lines with just a comment
# symbol=value
# BEGIN/END  - scope - 1 level
# LABEL:
# ALP,  ALP relative address, or ALP absolute address
# Question - is there a comma?, i.e.  ALP .+20  or  ALP, .+20
# =E expression  123,  =X123,  LABEL,  =C"12"
# DATA followed by =Xhhhhhhhhhhhh .... or =C"text"
# FINISH or FINISH, jump-to-address ;
#
# Add a nod to extended DataCode.
# ENTRY, NAME/SYMBOL/LABEL
# EXTERNAL, NAME/SYMBOL/LABEL
#
Kinds = enum.Enum("Kinds", ("unknown", "empty", "comment",
                            "alp_directive", "instruction", "expression",
                            "scope_begin", "scope_end",
                            "symbol", "label",
                            "data_directive", "data",
                            "entry_directive",      # place holder
                            "external_directive",   # place holder
                            "finish_directive"))

Status = enum.Enum("Status", ("okay", "warning", "error"))

Modes = enum.Enum("Modes", ("alp", "data"))


# Use for symbols and labels - does not contain own name.
#
Symbol = collections.namedtuple("Symbol", ("filename", "lineno", "value"))


# Used in various log outputs.
# Do once here for consistancy
#
timenow = datetime.datetime.now().strftime("%d-%m-%Y %H:%M:%S")


# -----------------------------------------------------------------------------
#
def parse(text, separator, *, drop_empty):
    """ Combines split/strip and optional drops empty items.
        separator may be a string or regular expression
    """
    parts = text.split(separator)

    result = []
    for part in parts:
        part = part.strip()
        if not drop_empty or len(part) > 0:
            result.append(part)
    return result


# -----------------------------------------------------------------------------
#
def is_symbol_definition(text: str) -> bool:
    """ Looking for ABCD = expression, however must avoid:
        =expression   as in =E stuff
        SETA  =X41,L  a valid instruction

        While it does not check the validity of the name in general,
        it does check for reserved, words "ALP", "DATA", etc.
    """

    eq = text.find("=")
    if eq <= 0:
        return False

    com = text.find(",")
    if com >= 0:
        return False

    return True


# -----------------------------------------------------------------------------
#
def value_to_bytes(value: int) -> bytes:
    """ Converts (and truncates) a signed integer to a 2 element bytes object
        in big endian representation.
         255   => '\x00\xff'
        -2     => '\xff\xfe'
        -32768 => '\x80\x00'
         32767 => '\x7f\xff'
    """
    return (value & 0xFFFF).to_bytes(2, 'big')


# -----------------------------------------------------------------------------
#
def bytes_to_hex_string(value: int | bytes) -> str:
    """ Converts (and truncates) a byte integer to an unsigned 2 digit hexadecimal
        string representation. If value is a bytes/byteArray, it converts each byte.
         255   => FF
        -1     => FF
        128    => 80
    """
    if isinstance(value, int):
        result = hex(value & 0xFF)[2:].upper()
        if len(result) < 2:
            result = '0' + result

    elif isinstance(value, (bytes, bytearray)):
        result = ""
        for b in value:
            t = hex(b)[2:].upper()
            if len(t) < 2:
                t = '0' + t

            result = result + t

    else:
        raise TypeError(
            "bytes_to_hex_string: must be int, bytes or bytearray, not %s" % type(value))

    return result


# -----------------------------------------------------------------------------
#
def value_to_hex(value: int) -> str:
    """ Converts (and truncates) an integer to a signed 4 digit hexadecimal
        representation.
         255   => 00FF
        -1     => FFFF
        -32768 => 8000

        This is equivilent to bytes_to_hex_string(value_to_bytes(value))
    """
    result = hex(value & 0xFFFF)[2:].upper()
    while len(result) < 4:
        result = '0' + result
    return result


# -----------------------------------------------------------------------------
#
def check(item, group) -> bool:
    """ Checks if item is in the group (tuple, list) and
        if it is, it sets check.position accordingly.
    """
    okay = item in group
    if okay:
        check.position = group.index(item)
    else:
        check.position = None
    return okay


check.position = None


# -----------------------------------------------------------------------------
#
def write_phx(file_info, jump_to, target, source):
    """ Write the  file_info to the target file as PHX
    """
    with open(target, 'w') as f:
        # No parity - for now
        #
        soh = "\x01"
        stx = "\x02"
        etx = "\x03"
        crlf = "\r\n"

        header = f"""\
Source: {source}{crlf}\
Date: {timenow}{crlf}\
dc1 version: {__version__}{crlf}"""

        f.write(soh)
        f.write(header)
        f.write(stx)
        f.write(crlf)

        count = 0       # line length count
        address = None  # Force first output to do Txxxx
        for line_item in file_info:
            code = line_item.get('code', None)
            if code is not None:
                if address != line_item['addr']:
                    # Need a T directive
                    address = line_item['addr']
                    if count > 0:
                        f.write(crlf)
                    f.write(f"T%s{crlf}" % value_to_hex(address))
                    count = 0

                for b in code:
                    f.write(bytes_to_hex_string(b))
                    count += 2
                    address += 1
                    if count >= 80:
                        f.write(crlf)
                        count = 0

        if count > 0:
            f.write(crlf)

        f.write(f"J%s{crlf}" % value_to_hex(jump_to))
        f.write(f"{etx}{crlf}")


# -----------------------------------------------------------------------------
#
def escape(data):
    """ Introduces escape chars as necessary into bytes/bytearray
        Returns a bytes object
    """

    esc = 0x1B

    if not isinstance(data, (bytes, bytearray)):
        raise TypeError("escape: must be bytes or bytearray, not %s" % type(value))

    work = bytearray()

    for b in data:
        if b == esc:
            work.append(esc)
        work.append(b)

    return bytes(work)


# -----------------------------------------------------------------------------
#
def write_ocb(file_info, jump_to, target, source):
    """ Write the  file_info to the target file as OCB, object compressed binary
        What is the escape character? Escape??
    """

    with open(target, 'wb') as f:
        # No parity - for now
        #
        esc = 0x1B

        # Pre-escaped bytes strings
        #
        soh = bytes((esc, 0x01))
        stx = bytes((esc, 0x02))
        etx = bytes((esc, 0x03))
        tdir = bytes((esc, ord('T')))
        jdir = bytes((esc, ord('J')))

        crlf = "\r\n"

        header = f"""\
Source: {source}{crlf}\
Date: {timenow}{crlf}\
dc1 version: {__version__}{crlf}"""

        f.write(soh)
        f.write(escape(bytes(header, encoding='utf-8')))
        f.write(stx)

        address = None  # Force first output to do Txxxx
        for line_item in file_info:
            code = line_item.get('code', None)
            if code is not None:
                if address != line_item['addr']:
                    # Need a T directive
                    address = line_item['addr']
                    f.write(tdir)
                    addr = escape(value_to_bytes(address))
                    f.write(addr)

                f.write(escape(code))
                address += len(code)

        f.write(jdir)
        addr = escape(value_to_bytes(jump_to))
        f.write(addr)
        f.write(etx)
        f.write(b'\r\n')


# -----------------------------------------------------------------------------
#
def assesmble_dc1(source, target, addr_map, listing, ocb=False):
    """ If ocb is False, output is Printable HeXadecimal (PHX)
        If ocb is True, output is Object Compressed Binary (OCB)
    """

    # Form a tuple out of a single string argument
    #
    if isinstance(source, str):
        source = (source, )

    # For inclusion in log, map and phx files.
    #
    source_name = ", ".join(source)

    # The orginal Data Code 1 assesmbler had two passes to resolve names.
    # So does this. Pass "0" is purely a pre-processing reformat pass.
    #
    # Each line is represented by a dictionary that initially includes the
    # raw text, line number, and default status. The line dictionary is
    # augmented in pass 1/2.
    #
    # The set of lines dictionaries are themselves held in a list.
    # Line numbers start from 1.
    #

    # Pass 0
    #
    file_info = []

    for filename in source:
        with open(filename, 'r') as f:
            file_lines = f.read().splitlines()

        for lineno, line in enumerate(file_lines, 1):
            # Status okay until we know it is an erroneous
            #
            line_item = {'filename': filename, 'lineno': lineno, 'text': line,
                         'kind': Kinds.unknown, 'status': Status.okay, 'message': ''}
            file_info.append(line_item)

    # No longer required - free up any memory
    #
    del file_lines

#   print(pp.pformat(file_info))

    initial_address = -28672    # 9000
    jump_to = initial_address   # default

    global_values = {}

    # Each pass does a lot of common stuff (at least for now)
    # However there is also a lots of:  if pass_no == 1   or
    # if pass_no == 2  checks as well.
    #
    for pass_no in (1, 2):
        print("pass %d ..." % pass_no)

        address = initial_address
        mode = Modes.alp

        local_values = None
        in_begin_end = False
        begin_start = None

        for line_item in file_info:
            # print(pp.pformat(line_item))

            filename = line_item['filename']
            lineno = line_item['lineno']    # local file line number
            text = line_item['text']

            # First identify comments and blank lines.
            # What about comments in data: =C"....//...."
            #
            parts = parse(text, "//", drop_empty=False)

            if len(parts) == 0:
                raise DataCodeOneAssesmblerException("%s:%d  Too few parts" % (filename, lineno))

            if len(parts) == 1 and len(parts[0]) == 0:
                # Blank line
                #
                line_item['kind'] = Kinds.empty
                continue

            if len(parts) >= 2 and len(parts[0]) == 0:
                # Comment line
                #
                line_item['kind'] = Kinds.comment
                continue

            line = parts[0]    # text sans any comments

            # As well as instructions (SETA 42,R and the like) we have:
            # symbol=value or expression
            # BEGIN/END  - scope - 1 level
            # LABEL:
            # ALP or   ALP, relative address or ALP, absolute address
            # =E expression  123,  =X123,  LABEL,  =C"12"
            # DATA (with address DATA, address) followed by
            # =Xhhhhhhhhhhhh .... or =C"texttexttext"
            # FINISH, jump address;
            #
            if line == "BEGIN":
                if in_begin_end == False:
                    line_item['kind'] = Kinds.scope_begin
                    if pass_no == 1:
                        local_values = {}
                        line_item['local'] = local_values
                    else:
                        local_values = line_item['local']
                    in_begin_end = True
                else:
                    #  DC1 did not allow this (as I recall).
                    line_item['status'] = Status.error
                    line_item['message'] = "Nested BEGIN"
                continue

            if line == "END":
                if in_begin_end == True:
                    line_item['kind'] = Kinds.scope_end
                    in_begin_end = False
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Unexpected END"
                continue

            if line.endswith(":"):
                # Label
                #
                name = line[0: -1].strip()
                if is_a_valid_name(name):
                    if in_begin_end:
                        check_values = local_values
                        gl = "local"
                    else:
                        check_values = global_values
                        gl = "global"

                    if name not in check_values:
                        line_item['kind'] = Kinds.label
                        symbol = Symbol(line_item['filename'], line_item['lineno'], address)
                        check_values[name] = symbol
                        line_item['addr'] = address

                    elif pass_no == 1:
                        # We expect duplicates on pass 2, only check on pass 1.
                        #
                        symbol = check_values[name]
                        original = f"{filename}:{symbol.lineno}"
                        line_item['status'] = Status.error
                        line_item['message'] = f"Duplicate {gl} name: {name} - initial declaration: {original}"

                else:
                    line_item['status'] = Status.error
                    line_item['message'] = f"Invalid label format: {is_a_valid_name.message}."
                continue

            if is_symbol_definition(line):  # Need to avoid  =E 123, also SETA =X22,L
                # Processing is similar to label
                #
                line_item['kind'] = Kinds.symbol

                eq = line.find("=")
                name = line[0: eq].strip()
                expr = line[eq + 1:].strip()

                if is_a_valid_name(name):
                    if in_begin_end:
                        check_values = local_values
                        gl = "local"
                    else:
                        check_values = global_values
                        gl = "global"

                    # Check for duplicates.
                    # Note pass 1 only, we expect duplicates on pass 2.
                    #
                    if pass_no == 1 and name in check_values:
                        symbol = check_values[name]
                        original = f"{filename}:{symbol.lineno}"
                        line_item['status'] = Status.error
                        line_item['message'] = f"Duplicate {gl} name: {name} - initial declaration: {original}"

                    value = evaluate_expression(expr, address,
                                                global_values, local_values)
                    line_item['value'] = value
                    symbol = Symbol(line_item['filename'], line_item['lineno'], value)
                    check_values[name] = symbol

                    # Symbol expressions only needs to successfully be resolved in pass 2.
                    #
                    if pass_no == 2 and value is None:
                        line_item['status'] = Status.error
                        line_item['message'] = f"{evaluate_expression.message}."

                else:
                    line_item['status'] = Status.error
                    line_item['message'] = f"Invalid name format: {is_a_valid_name.message}."
                continue

            if line.startswith("ALP"):
                line_item['kind'] = Kinds.alp_directive
                mode = Modes.alp
                if line == "ALP":
                    # Stard-alone
                    #
                    address += address % 2   # round up

                elif line.startswith("ALP,"):  # ALP <space> or ALP, ?????
                    if pass_no == 1:
                        new_address = evaluate_expression(line[4:], address,
                                                          global_values, local_values)
                        if new_address is not None:
                            new_address += new_address % 2
                            address = new_address
                            line_item['new_address'] = new_address
                        else:
                            line_item['new_address'] = None
                            line_item['status'] = Status.error
                            line_item['message'] = f"ALP directive must resolve in pass 1: {evaluate_expression.message}"
                    else:  # pass 2
                        new_address = line_item['new_address']
                        if new_address is not None:
                            address = new_address
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Mal-formed ALP directive"

                continue

            if line.startswith("DATA"):
                # Virtually identical to ALP
                #
                line_item['kind'] = Kinds.data_directive
                mode = Modes.data
                if line == "DATA":
                    # Stand-alone
                    #
                    pass

                elif line.startswith("DATA,"):
                    if pass_no == 1:
                        new_address = evaluate_expression(
                            line[5:], address, global_values, local_values)
                        if new_address is not None:
                            address = new_address
                            line_item['new_address'] = new_address
                        else:
                            line_item['new_address'] = None
                            line_item['status'] = Status.error
                            line_item['message'] = f"DATA directive must resolve in pass 1: {evaluate_expression.message}"
                    else:  # pass 2
                        new_address = line_item['new_address']
                        if new_address is not None:
                            address = new_address
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Mal-formed DATA directive"

                continue

            if line.startswith("ENTRY"):
                line_item['status'] = Status.error
                line_item['message'] = "Extended DataCode ENTRY directive not supported ... yet."
                continue

            if line.startswith("EXTERNAL"):
                line_item['status'] = Status.error
                line_item['message'] = "Extended DataCode EXTERNAL directive not supported ... yet."
                continue

            if line.startswith("FINISH") and line.endswith(";"):
                line_item['kind'] = Kinds.finish_directive
                # remove spaces?
                if line == "FINISH;":
                    # Stand-alone
                    #
                    pass  # end parse

                elif line.startswith("FINISH,"):
                    jump_address = evaluate_expression(line[7:-1], address,
                                                       global_values, local_values)
                    if jump_address is not None:
                        jump_to = jump_address
                    elif pass_no == 2:
                        line_item['status'] = Status.error
                        line_item['message'] = "FINISH directive unresolved."
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Mal-formed FINISH directive"

                continue

            # Which mode?
            #
            if mode == Modes.data:
                # We are looking for  =Xhhhhhh... or =C"tttttt...."
                # For =C did dc1 assembler automatically append a 0 string terminator?
                #

                if line.startswith("=X"):
                    line_item['kind'] = Kinds.data
                    line_item['addr'] = address
                    hex_line = line[2:]
                    n = len(hex_line)
                    if n % 2 == 1:
                        address += (n + 1) // 2      # Even if invalid, this gives rough addressing
                        line_item['status'] = Status.error
                        line_item['message'] = "Odd number of hex digits"
                        continue

                    data = bytearray()
                    for j in range(n // 2):
                        # Need to catch errors
                        data.append(int(hex_line[2 * j:2 * j + 2], 16))

                    line_item['code'] = bytes(data)
                    address += len(line_item['code'])

                elif line.startswith("=C"):
                    line_item['kind'] = Kinds.data
                    line_item['addr'] = address
                    char_line = line[2:]
                    n = len(char_line)
                    if n < 2 or not char_line.startswith('"') or not char_line.endswith('"'):
                        address += (n - 2)   # Even if invalid, this gives rough addressing
                        line_item['status'] = Status.error
                        line_item['message'] = "Miss-quoted string"
                        continue

                    line_item['code'] = bytes(char_line[1:-1], encoding='UTF-8')
                    address += len(line_item['code'])

                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Unknown data type"
                continue

            # Must be instruction -r expression
            #
            if line.startswith("=E "):
                # Expression
                #
                line_item['kind'] = Kinds.expression
                if pass_no == 2 and address != line_item['addr']:
                    raise DataCodeOneAssesmblerException("pass1/pass2 address mis-match")
                line_item['addr'] = address

                value = evaluate_expression(line[3:], address,
                                            global_values, local_values)
                if value is not None:
                    line_item['code'] = value_to_bytes(value)
                    line_item['kind'] = Kinds.expression
                    if value < -32768 or value > 32767:
                        line_item['status'] = Status.warning
                        line_item['message'] = "Expression value (%d) out of range" % value

                elif pass_no == 2:
                    line_item['status'] = Status.error
                    line_item['message'] = f"{evaluate_expression.message}."

                if address < -0x8000:
                    line_item['status'] = Status.error
                    line_item['message'] = "Address less that =X8000"
                elif address >= 0x7FFF:
                    line_item['status'] = Status.error
                    line_item['message'] = "Address exceeds =X7FFF"

                address += 2
                continue

            # Must be an instruction (or nonsense)
            #
            if pass_no == 2 and address != line_item['addr']:
                msg = "%s:%d: pass1/pass2 address mis-match"
                raise DataCodeOneAssesmblerException(msg % (filename, lineno))
            line_item['addr'] = address

            if pass_no == 1:
                # Leave instructions entirely to pass 2, apart from the address increment.
                #
                address += 2
                continue

            parts = parse(line, " ", drop_empty=True)
            cmd = parts[0]
            remaining = line[len(cmd):]
            operands = parse(remaining, ",", drop_empty=False)

            if len(operands) >= 2:
                indices = ("P", "R", "S", "T",
                           "PB", "RB", "SB", "TB",
                           "I", "RI", "SI", "TI",
                           "L", "A", "LC")
                valid_index = check(operands[1], indices)
                index = check.position
            else:
                index = 0    # implied P

            # Local fuctions to encode various instruction types.
            #
            def regular(action, idx):
                """
                """
                offset = evaluate_expression(operands[0], address,
                                             global_values,
                                             local_values)
                if idx < 4:
                    min_value = -254
                    max_value = +254
                else:
                    # byte mode
                    min_value = -127
                    max_value = +127

                if offset is not None and (idx % 4) == 0:
                    # Relative to P
                    offset = offset - address - 2

                if offset is not None and min_value <= offset <= max_value:
                    line_item['kind'] = Kinds.instruction

                    idx_reg = (idx % 4) * 0x0200
                    sign = 0x0000 if offset >= 0 else 0x0100

                    if idx < 4:
                        lsb = abs(offset) & 0xFE
                    else:
                        lsb = abs(offset) * 2 + 1

                    code = (action * 0x0800) + idx_reg + sign + lsb
                    line_item['code'] = value_to_bytes(code)
                elif offset is None:
                    line_item['status'] = Status.error
                    line_item['message'] = f"Expression not defined: {evaluate_expression.message}"
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Offset value out of range"

            def jumps(action, qualifier, idx):
                """ Process J, JS, JLT, JGT etc.
                """
                offset = evaluate_expression(operands[0], address,
                                             global_values, local_values)
                min_value = -254
                max_value = +254

                if offset is not None and (idx % 4) == 0:
                    # Relative to P
                    offset = offset - address - 2

                if offset is not None and min_value <= offset <= max_value:
                    line_item['kind'] = Kinds.instruction

                    extra = (qualifier % 4) * 0x0200
                    sign = 0x0000 if offset >= 0 else 0x0100
                    lsb = (abs(offset) & 0xFE) + (idx // 8)

                    code = (action * 0x0800) + extra + sign + lsb
                    line_item['code'] = value_to_bytes(code)
                elif offset is None:
                    line_item['status'] = Status.error
                    line_item['message'] = f"Expression not defined: {evaluate_expression.message}"
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Offset value out of range"

            def literal(action, reg):
                """ Generate literal codes
                """
                offset = evaluate_expression(operands[0], address,
                                             global_values, local_values)
                if offset is not None and 0 <= offset <= 255:
                    line_item['kind'] = Kinds.instruction
                    code = 0xE000 + (reg * 0x0800) + (action * 0x0100) + offset
                    line_item['code'] = value_to_bytes(code)
                elif offset is None:
                    line_item['status'] = Status.error
                    line_item['message'] = f"Expression not defined: {evaluate_expression.message}"
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Literal value out of range 0 .. 255"

            def shifts(direction, register, mode):
                """ Generate shift codes
                    These are very speculative
                    direction 0=Left, 1=Right
                    register  0=A, 1=R, 2=S, 3=T
                    mode      0=L, 1=A, 2=LC
                """
#               print (direction, register, mode)

                offset = evaluate_expression(operands[0], address,
                                             global_values, local_values)

                min_value = 1
                max_value = 15 if mode < 2 else 1

                if offset is not None and min_value <= offset <= max_value:
                    line_item['kind'] = Kinds.instruction
                    if mode == 2:
                        # For 1,LC shifts, set shift value set to 0 with an
                        # implied shift value of 1.
                        #
                        mode = 0
                        offset = 0

                    code = 0xE740 + (register * 0x0800) + (direction * 0x0020) + (mode * 0x0010) + offset
                    line_item['code'] = value_to_bytes(code)
                elif offset is None:
                    line_item['status'] = Status.error
                    line_item['message'] = f"Expression not defined: {evaluate_expression.message}"
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Shift alue out of range 1 .. %d" % max_value

            #
            # end of local inner functions

            # Check for eack instruction command/register combo.
            #
            if check(cmd, ("SETA", "SETR", "SETS", "SETT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8:
                    regular(0 + check.position, index)
                elif index == 12:
                    literal(0, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("STRA", "STRR", "STRS", "STRT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8:
                    regular(4 + check.position, index)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("ADDA", "ADDR", "ADDS", "ADDT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8:
                    regular(8 + check.position, index)
                elif index == 12:
                    literal(1, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("CMPA", "CMPR", "CMPS", "CMPT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8:
                    regular(12 + check.position, index)
                elif index == 12:
                    literal(3, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("SUBA", "SUBR", "SUBS", "SUBT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8 and check.position < 2:
                    regular(16 + check.position, index)
                elif index == 12:
                    literal(2, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("ANDA", "ANDR", "ANDS", "ANDT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8 and check.position < 2:
                    regular(18 + check.position, index)
                elif index == 12:
                    literal(4, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("NEQA", "NEQR", "NEQS", "NEQT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8 and check.position < 2:
                    regular(20 + check.position, index)
                elif index == 12:
                    literal(5, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("IORA", "IORR", "IORS", "IORT")):
                if index is None:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"
                elif index < 8 and check.position < 2:
                    regular(22 + check.position, index)
                elif index == 12:
                    literal(6, check.position)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif cmd in ("J"):
                if index is not None and (0 <= index < 4 or 8 <= index < 12):
                    jumps(24, index % 4, index)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif cmd in ("JS"):
                if index is not None and (0 <= index < 4 or 8 <= index < 12):
                    jumps(25, index % 4, index)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("JVS", "JVN", "JCS", "JCN",
                             "JLT", "JGE", "JEQ", "JNE",
                             "JNGA", "JPZA", "JEZA", "JNZA",
                             "JNGR", "JPZR", "JEZR", "JNZR",
                             "JNGS", "JPZS", "JEZS", "JNZS")):
                if index in (0, 8):
                    # JVS/JLT/JNGA/JNGR/JNGS etc are essentially the same etc.
                    jumps(26, check.position % 4, index)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif cmd in ("MLT"):
                if index is not None and index < 8:
                    regular(27, index)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("SHLA", "SHLR", "SHLS", "SHLT")):
                if index in (12, 13, 14):   # Is SHLX  n,A allowed ??
                    shifts(0, check.position, index - 12)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif check(cmd, ("SHRA", "SHRR", "SHRS", "SHRT")):
                if index in (12, 13, 14):
                    shifts(1, check.position, index - 12)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Invalid index register"

            elif cmd in ("SETL"):
                line_item['kind'] = Kinds.instruction
                level = evaluate_expression(operands[0], address,
                                            global_values, local_values)
                if level is not None and level >= 0 and level < 4:
                    line_item['code'] = value_to_bytes(0xFF00 + level)
                else:
                    line_item['status'] = Status.error
                    line_item['message'] = "Level out of range"

            elif cmd in ("SETK"):
                line_item['kind'] = Kinds.instruction
                line_item['code'] = value_to_bytes(0xFF21)

            elif cmd in ("CLRK"):
                line_item['kind'] = Kinds.instruction
                line_item['code'] = value_to_bytes(0xFF20)

            elif cmd in ("NUL"):
                line_item['kind'] = Kinds.instruction
                line_item['code'] = value_to_bytes(0xFFFF)

            else:
                line_item['status'] = Status.error
                line_item['message'] = "Unknown instruction/op code"

            if address >= 0x7FFF:
                line_item['status'] = Status.error
                line_item['message'] = "Address exceeds =X7FFF"
            address += 2

    # end pass loop

    errors = 0
    warnings = 0
    no_code = b"--"
    gap = " " * 21

    with open(listing, 'w') as f:
        f.write(f"Source: {source_name}\n")
        f.write(f"Date:   {timenow}\n")
        f.write("\n")
        for line_item in file_info:

            filename = line_item['filename']
            lineno = line_item['lineno']    # local file number

            kind = line_item['kind']
            if kind in (Kinds.instruction, Kinds.expression):

                addr = line_item.get('addr', None)
                code = line_item.get('code', no_code)

                text = " %4d  %4s  %4s    %s" % (lineno, value_to_hex(addr),
                                                 bytes_to_hex_string(code),
                                                 line_item['text'])
                f.write(text.rstrip())
                f.write('\n')

            elif kind == Kinds.data:
                addr = line_item.get('addr', None)
                data = line_item.get('code', no_code)
                text = line_item['text']

                n = (len(data) + 1) // 2
                for t in range(n):
                    a = 2 * t
                    code = data[a:a + 2]    # will auto truncate

                    text = " %4d  %4s  %-4s      %s" % (lineno, value_to_hex(addr),
                                                        bytes_to_hex_string(code),
                                                        text)
                    f.write(text.rstrip())
                    f.write('\n')
                    addr += 2
                    text = ''

            elif kind == Kinds.label:
                addr = line_item.get('addr', 0)
                xaddr = value_to_hex(addr)
                text = line_item['text']
                f.write(f" {lineno:4}  {xaddr:4}          {text}\n")

            else:
                # None coding line
                text = " %4d                %s" % (lineno, line_item['text'])
                f.write(text.rstrip())
                f.write('\n')

            # Output any error messages
            #
            message = line_item.get('message', '????')
            if line_item['status'] == Status.error or \
               line_item['kind'] == Kinds.unknown or \
               ('code' in line_item and 'addr' not in line_item):
                errors += 1

                f.write("*** Error: %s\n" % message)

                print(line_item['text'])
                print("%s:%d %sError: %s%s" %
                      (filename, lineno, error_col, message, reset_col))

            elif line_item['status'] == Status.warning:
                warnings += 1

                f.write("*** Warning: %s\n" % message)

                print(line_item['text'])
                print("%s:%d %sWarning: %s%s" %
                      (filename, lineno, warn_col, message, reset_col))

        # end line

        if in_begin_end == True:
            f.write("*** Warning: %s\n" % "Missing END")

            # There is no line number
            print("%s:%d %sWarning: %s%s\n" %
                  (filename, lineno + 1, warn_col, "Missing END", reset_col))
            warnings += 1

        f.write("\n")
        if errors == 0:

            f.write(f"Global Names (also available in {addr_map}):\n")
            sorted_names = sorted(global_values.keys())
            for name in sorted_names:
                symbol = global_values[name]
                value = symbol.value
                gap = " " * (14 - len(name))
                f.write("%s==X%s%s // %s\n" % (name, value_to_hex(value), gap, value))
            f.write("\n")

            if warnings == 0:
                f.write("Assembly successfull\n")
            else:
                f.write("Assembly complete with %d warnings\n" % warnings)

        else:
            f.write("Assembly failed\n")
            f.write("Errors:   %d\n" % errors)
            f.write("Warnings: %d" % warnings)
        f.write("\n")

    # end log output
    print(f"Listing file: {listing}")

#   print()
#   print(pp.pformat (file_info))

    if errors == 0:
        if warnings == 0:
            print("Assembly successfull")
        else:
            print("Assembly complete with %d warnings" % warnings)

        if ocb:
            write_ocb(file_info, jump_to, target, source_name)
        else:
            write_phx(file_info, jump_to, target, source_name)

        print("Output file: %s" % target)

        # Write map file.
        #
        with open(addr_map, 'w') as f:
            f.write("// %s\n" % addr_map)
            f.write("// Source: %s\n" % source_name)
            f.write("// Date: %s\n" % timenow)
            f.write("\n")
            sorted_names = sorted(global_values.keys())
            for name in sorted_names:
                symbol = global_values[name]
                value = symbol.value
                gap = " " * (14 - len(name))
                f.write("%s==X%s%s // %s\n" % (name, value_to_hex(value), gap, value))
            f.write("\n")
            f.write("// end\n")

    else:
        print("Assembly failed")
        print("Errors:   %d" % errors)
        print("Warnings: %d" % warnings)
        os._exit(4)


# -----------------------------------------------------------------------------
#
def print_version(ctx, param, value):
    """ Click parser helper function """
    if not value or ctx.resilient_parsing:
        return

    vi = sys.version_info
    print("dc1 version: %s  (python %s.%s.%s)" % (__version__, vi.major, vi.minor, vi.micro))
    ctx.exit()


# -----------------------------------------------------------------------------
#
context_settings = dict(help_option_names=['--help', '-h'],
                        terminal_width=108,
                        max_content_width=112)


@click.command(context_settings=context_settings,
               epilog="""
\b\bExample:

dc1  rom.map  example.dc1

This generates output files example.phx, example.log and example.map.

""")
# -----------------------------------------------------------------------------
#
@click.option('--ocb', '-b',
              is_flag=True,
              help="""\
Select OCB output format in lieu of PHX.
""")
# -----------------------------------------------------------------------------
#
@click.option('--output', '-o',
              help="""
Define the filename prefix for all output files, i.e. the phx (or ocb) file, \
the log file and the map file. If not specified, the output prefix is based \
on the first source filename ending with .dc1 if it exists, otherwise it based \
on the first source filename.
""")
# -----------------------------------------------------------------------------
#
@click.option('--version', '-V',
              is_flag=True,
              callback=print_version,
              expose_value=False, is_eager=True,
              help="Show dc1 version and exit.")
#
@click.argument('source', nargs=-1, required=True)
#
# -----------------------------------------------------------------------------
#
def cli(ocb, output, source):
    """ Assembles one or more DataCodeOne source file(s) to phx or ocb.
    """
    if output is not None:
        # User has specified the prefix name.
        #
        prefix = output
    else:
        # prefix name is from the first *.dc1 file if it exists
        # otherwise is based on the first file.
        #
        prefix = None
        for source_name in source:
            if source_name.endswith(".dc1"):
                prefix = source_name[0:-4]
                break

        if prefix is None:
            # Just go with first file - check if a map file.
            #
            prefix = source[0]
            if prefix.endswith(".map"):
                prefix = prefix[0:-4]

    if ocb:
        target = prefix + ".ocb"
    else:
        target = prefix + ".phx"
    listing = prefix + ".log"
    addr_map = prefix + ".map"

    assesmble_dc1(source, target, addr_map, listing, ocb=ocb)


# -----------------------------------------------------------------------------
# Set env variables for click and python 3 (does no harm for python 2)
# Command line entry point for setup
#
def call_cli():
    """ cli wrapper to setup click friendly envrionment variables.
        Would also be the console scripts entry point if this ever became a package.
    """
    os.environ['LANG'] = 'en_US.utf8'
    os.environ['LC_ALL'] = 'en_US.utf8'
    cli()

# end
