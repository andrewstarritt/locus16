"""
Burm rom from phx file
"""

# SPDX-FileCopyrightText: 2022-2025 Andrew C. Starritt
# SPDX-License-Identifier: GPL-3.0-only

import click
import functools
import os
import sys
import traceback

# Add a little colour
#
red = b'\x1b[31;1m'.decode(encoding='utf8')
green = b'\x1b[32;1m'.decode(encoding='utf8')
yellow = b'\x1b[33;1m'.decode(encoding='utf8')
reset = b'\x1b[00m'.decode(encoding='utf8')


# print to standard error.
#
errput = functools.partial(print, file=sys.stderr)


# -----------------------------------------------------------------------------
#
def burn(source, target, filename):
    """
    """
    soh = "\x01"
    stx = "\x02"
    etx = "\x03"
    cr = "\x0d"
    lf = "\x0a"

    phx = source.read()

    # Certain items must exist in the PHX for this to be considered valid.
    #
    p1 = phx.find(soh)
    if p1 == -1:
        raise ValueError(f"{filename}: Missing SOH")

    p2 = phx.find(stx, p1)
    if p2 == -1:
        raise ValueError(f"{filename}: Missing STX")

    p3 = phx.find('T8000', p2)
    if p3 == -1:
        raise ValueError(f"{filename}: Missing T8000 directive")

    p4 = phx.find('J8000', p3)
    if p4 == -1:
        raise ValueError(f"{filename}: Missing J8000 directive")

    p5 = phx.find(etx, p4)
    if p5 == -1:
        raise ValueError(f"{filename}: Missing ETX")

    output = bytearray()

    # Fill rom with C102, i.e.  J .+0
    #
    while len(output) < 4096:
        output.append(0xC1)
        output.append(0x02)

    nibble = 0
    byte = 0
    addr = 0     # relative to start

    # The useful PHX is in phx[p3+5:p4]
    #
    index = p3 + 5
    while index < p4:
        x = phx[index]
        index += 1

        if x in (cr, lf):
            pass

        elif x in "0123456789ABCDEF":
            if x in "0123456789":
                v = ord(x) - ord("0")
            else:
                v = ord(x) - ord("A") + 10

            byte = (byte << 4) + v
            nibble += 1

            if nibble == 2:
                if addr < 0 or addr >= len(output):
                    load_addr = addr + 0x8000
                    raise ValueError(f"{filename}: load address 0x{load_addr:04X} exceeds 0x9000")

                output[addr] = byte
                addr += 1
                nibble = 0
                byte = 0

        elif x == "T":
            if nibble != 0:
                raise ValueError(f"{filename}: T directive between hex pair")
            t_directive = 0
            for _ in range(4):
                x = phx[index]
                index += 1
                if x in "0123456789":
                    v = ord(x) - ord("0")
                elif x in "ABCDEF":
                    v = ord(x) - ord("A") + 10
                else:
                    raise ValueError(f"{filename}: T directive - non hex char '{x}'")

                t_directive = (t_directive << 4) + v

            addr = t_directive - 0x8000
#           print (f"{index} T directive 0x{t_directive:04X}  {addr}")

        else:
            raise ValueError(f"{filename}: non PHX character '{x}'")

    if nibble > 0:
        raise ValueError(f"{filename}: odd number of PHX characters")

    target.write(output)


# -----------------------------------------------------------------------------
#
context_settings = dict(help_option_names=['--help', '-h'],
                        terminal_width=108,
                        max_content_width=112)


@click.command(context_settings=context_settings,
               epilog="""
\b

""")
@click.argument('phx_file', nargs=1, required=True)
@click.argument('rom_file', nargs=1, required=True)
def cli(phx_file, rom_file):
    """ Uses the specified PHX_FILE to "burn" the specified ROM_FILE.
        The PHX file must start loading at 8000 and nominally
        jump to 9000.
    """
    try:
        with open(phx_file, 'r') as source:
            with open(rom_file, 'wb') as target:
                burn(source, target, phx_file)
        print(f"{rom_file} burn complete")

    except Exception as e:
        en = e.__class__.__name__
        errput(f"{red}{en}{reset}: {e}")
        errput(traceback.format_exc(limit=-2))
        os._exit(1)


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
