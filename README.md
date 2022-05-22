#  <span style='color:#ee6600'>Marconi Locus 16 EMUlator</span> &nbsp; &nbsp; ![](locus16.png?raw=true)

[Introduction](#intro)<br>
[General Information](#gen_info)<br>
[Status](#status)<br>
<br>
<br>
![](locus16_crate.png?raw=true)
<br>

# <a name="intro"></a><span style='color:#ee6600'>Introduction</span>

The Locus 16 emulator (currently incomplete) provides emulation of the
Marconi Radar Systems Ltd. Locus 16 mini computer.

This repo provides the emulator source code (C++) and associated Makefile
needed to build the emulator itself (binary file locus16) together with
a (python3) DataCode1 assembler program, dc1.

# <a name="gen_info"></a><span style='color:#ee6600'>General Information</span>

## <span style='color:#a0a000'>Configuration</span>

Currently the devices and peripheral in the Locus 16 crate are currently
hard coded.
These are:
 - Primary ALP1 Processor
 - Memory and memory mapping controller.
 - ROM module.
 - A terminal pheripheral
 - A tape reader pheripheral
 - A tape punch pheripheral
 - Two serial channels (one input, one output) connected to the terminal
 - One serial channel (input) connected to the tape reader.
 - One serial channel (output) connected to the tape punch.

Ideally, in future, the crate configuration would be specified using a
configuration file.
The current configuration is defined in the file locus16.ini.

### <span style='color:#a0a000'>Build Environment</span>

#### <span style='color:#a0a000'>The Emulator</span>

The emulator has been built and tested on Linux
(specifically CentOS Linux release 7.9.2009) using
gcc (GCC) 4.8.5.
While most of the code is OS independent, the peripherals and the the terminal in particular are very Linux specific.

Only one additional library is included. While not used yet, the emulator
program still calls up and links with a simple INI file reader.

On RedHat and similar distros (e.g. Rocky, CentOS, Fedora) run:

    dnf install  inih-devel

and for older distros:

    yum install  inih-devel

On Debian and similar distros (e.g. Ubuntu, Kbuntu) run:

    apt install libinih-dev

I have no info for package managers used by other distros.
Details of this library may be found at [here](https://github.com/benhoyt/inih)

#### <span style='color:#a0a000'>The Assembler</span>

The assembler is written in python using 3.9.2.
I suspect any python version >= 3.6 would do.
The only additonal module directly used other than standard python modules
is click (6.7).

    pip3 install click

should be sufficient.

# <a name="status"></a><span style='color:#ee6600'>Status</span>

## <span style='color:#a0a000'>General</span>

My memory is less than 100% perfect.
Where as I can recall some details perfectly, others are a little hazy.
Well, it has been 35+ years since I last used an actual Locus&nbsp;16 machine.
This section includes details what's missing, guessed and/or left to do.

## <span style='color:#a0a000'>Emulator Command Line</span>

The emulator provide a command line interface (separate from the terminal, 
see below).
Type help (actually "he" on its own will do) for a list of commands.

Features:
 - Memory may be access and decompiled.
 - Execution my be iterrupted with ^C (Control + C)
 - Break point may be set (upto 40), cleared and listed.

The is no capability to set memory (yet).

## <span style='color:#a0a000'>ROM Loader</span>

The rom.dc1 program is used to create rom.dat that gets loaded into the ROM.
Currently rom.dat is hard-coded into the emulator program.

The ROM program uses the serial channel connected to the tape reader to load
and then runs the program.
The file to be "loaded" into the tape reader is passed as an argument to the
locus16 program.
Currently, the ROM loader only loads PHX files (and it ignores parity).
Loading OCB files to follow.


## <span style='color:#a0a000'>Instruction Set</span>

Both the assembler and the emulator only support ALP1 instructions.
The most pressing issue for the emulator/assembler in the instruction set
and the associated op codes.

The instruction set is detailed in DataCodeOne.pdf, and the associated
operator codes are defined in alp1_op_codes.ods and/or alp1_op_codes.xlsx.
Where I could not recall the op code, I have done a best guess, or just
plain made up some op codes.
This is particularly true for the shift instructions.

Also "made-up" was not only the specific op codes for the post CMP instruction
conditional jumps and the trigger test jumps, but the relation between them
as well.
For example, JLT is effectively the same as JCN, JCS, JVN or JVS however
I can't recall this mapping.

The mapping used at the moment is:

    JLT is the same as JVS
    JGE is the same as JVN
    JEQ is the same as JCS
    JNE is the same as JCN


The JEZA, JNZA, JPZA and JNGA (ditto the R ans S registers) instructions need
an op code, however even if made up, there does not seem to be enough op-code
space for these instructions.
I have assumed that  __SETA ....__  does an implicit __CMPA 0,L__ , and these instructions are just synonyms for JLT, JGE, JEQ or JNE.

## <span style='color:#a0a000'>Assembler</span>

The assembler, dc1, is written in python. It is strictly a DataCode1
assembler as opposed to an Extended Data Code assembler, i.e. it generates
a loadable PHX output file as opposed to an intermediate object file.
Also it only handles APL1 intructions.

The main issues are (and my best guess):
- The format of the ALP directive. Is it "ALP address" or "ALP, address"?
  Similar applies to the DATA directive.
  Curently dc1 accepts the "ALP, address" format.
- How is the jump to or J directive defined.
  Was it "FINISH, jump-to-address" ?
  Currently dc1 just generates a jump to =X9000*.
- In OCB, object compressed binary, what whis the escape charater.
  Currently we use =X1B.
- Should data strings, =C"....." include an implicit =X00 at the end
  of the string.
  Curently dc1 does _not_ add a zero byte.

The PHX output is 7-bit, i.e. there is no parity bit.

__* Note:__ As with DataCodeOne, within this document, hexadecimal number are
denoted by the prefix =X.

## <span style='color:#a0a000'>ALP Processor</span>

While only one APL1 process has been hard coded, the emulator could support
two ALP processors and could be modified to support more.
I do not recall more that two ALP processors ever being used in the same crate.

Apart from the op-codes previously mentioned, the missing details here are
the memory-mapped registers addresses.
These are also best guess / made up.
For the primary ALP, they are:

| Reg  |   L0   |   L1   |   L2   |   L3   |
|:----:|:------:|:------:|:------:|:------:|
| P    | =X7F02 | =X7F12 | =X7F22 | =X7F32 |
| A    | =X7F04 | =X7F14 | =X7F24 | =X7F34 |
| R    | =X7F06 | =X7F16 | =X7F26 | =X7F36 |
| S    | =X7F08 | =X7F18 | =X7F28 | =X7F38 |
| T    | =X7F0A | =X7F1A | =X7F2A | =X7F3A |

Were the triggers (V, C, K) and indeed the currently level memory mapped?
Currently they are not mapped.

Lastly, there is no interrupt mechanism (nor a clock device to create interrupts).

## <span style='color:#a0a000'>Memory</span>

The memory is implemented as a "single card" as opposed to a number of
individual memory cards.

There is a total of 303.1 kB, in 74 by 4096-byte (2k word) blocks.

As I recall, there were two memory controller types, used to convert
a 16 bit address processor address to a 20 bit physical address.
In both types, I think the mapping of the address ranges =X9000 to =X1FFF
and =X6000 to =X6FFF was fixed,
while the address range mapping of =X2000 to =X5FFF was controllable
via mapping control register.
The mapping control was for each 4096-byte block, i.e.
for =X2000 to =X2FFF, =X3000 to =X3FFF, =X4000 to =X4FFF,
and =X5000 to =X5FFF.

For the emulator, I rolled my own.
The memory mapping control register is (arbitarily) defined to be at
located =X7B00 for the primary ALP, and =X7B02 for a secondary ALP.

The control register is divided into 4 nibbles.
Each nibble holds a value in the range 0 to 15.

- Nibble 1 controls the mapping for =X2000 to =X2FFF
- Nibble 2 controls the mapping for =X3000 to =X3FFF
- Nibble 3 controls the mapping for =X4000 to =X4FFF
- Nibble 4 controls the mapping for =X5000 to =X5FFF

Nibble 1 is the most signifficant nibble for the control register,
while nibble 4 is the least significant nibble.
The default register values are =X0000.


|   0  |   1  |   2  |  . . . |  13  |  14  |  15  |
|:----:|:----:|:----:|:------:|:----:|:----:|:----:|
| 6xxx |
| 5xxx | 5xxx | 5xxx |  . . . | 5xxx | 5xxx | 5xxx |
| 4xxx | 4xxx | 4xxx |  . . . | 4xxx | 4xxx | 4xxx |
| 3xxx | 3xxx | 3xxx |  . . . | 3xxx | 3xxx | 3xxx |
| 2xxx | 2xxx | 2xxx |  . . . | 2xxx | 2xxx | 2xxx |
| 1xxx |
| 0xxx |
| Fxxx |
| Exxx |
| Dxxx |
| Cxxx |
| Bxxx |
| Axxx |
| 9xxx |


## <span style='color:#a0a000'>Serial Channels</span>

There are four serial devices, two input and two output.
Two are allocated to the terminal, one to the reader and one to the tape punch.

Each serial device has a status word and a data word.
When the device has data ready to be read, or or ready to accept data for
output, the most significant four bits of the status word are set to 12 (=XC),
i.e.:

    (status-register-value  AND  =XF000) equal to  =XC000

One the device is ready, a byte may be read from or written to the data word.
If the device is __not__ ready, read/write actions are undefined.

|  No. | Type   | Status   | Data   |
|:----:|:------:|:--------:|:------:|
|  1   | Input  | =X7B10   | =X7B12 |
|  2   | Output | =X7B14   | =X7B16 |
|  3   | Input  | =X7B18   | =X7B1A |
|  4   | Output | =X7B1C   | =X7B1E |

The read and write status registers and data registers are all arbitarty
choosen.

## <span style='color:#a0a000'>Terminal Simulator</span>

The terminal, or Visual Display Unit (VDU), opens as an xterm.

Serial channels 1 and 2 can be used to access the terminal.

## <span style='color:#a0a000'>Tape Reader</span>

The tape reader opens and reads from the file passed to the emulator as the
first parameter.

Serial channel 3 can be used to access the tape reader.

The ROM loader attempts to load the program presented to the rape reader.

## <span style='color:#a0a000'>Tape Punch</span>

The tape punch  creates and writes to the file passed to the emulator program
as the second parameter if porvided, otherwise to the file punchout.txt

Serial channel 4 can be used to access the tape punch.

## <span style='color:#a0a000'>TODO</span>

- Real time clock (at =X7Cxx)


<font size="-1">Last updated: Sun May 22 14:43:24 AEST 2022</font>
<br>
