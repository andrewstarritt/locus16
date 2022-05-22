/* diagnostics.cpp
 */

#include "diagnostics.h"

#include <stdio.h>
#include <string.h>

#include "locus16_common.h"

using namespace L16E;

//------------------------------------------------------------------------------
//
Diagnostics::Diagnostics (DataBus* dataBusIn) :
   dataBus (dataBusIn)
{
   this->breakCount = 0;
   for (int j = 0 ; j < ARRAY_LENGTH(this->breakList); j++) {
      this->breakList [j] = 0;
   }
}

//------------------------------------------------------------------------------
//
Diagnostics::~Diagnostics () { }


//------------------------------------------------------------------------------
// static
char* Diagnostics::hex (const Int16 x)
{
   // All a bit nasty, but it gets the job done.
   //
   static int r = 0;
   static char buffer [8][6];
   r = (r+1)%8;

   snprintf (buffer[r], 6, "%04X", x & 0xFFFF);
   return buffer[r];
}

// ms 4 bits - basic op 0-4
//
static const char* cmdSet [16] = {
   "SET",    // 0
   "SET",    // 1
   "STR",    // 2
   "STR",    // 3
   "ADD",    // 4
   "ADD",    // 5
   "CMP",    // 6
   "CMP",    // 7
   "SUB",    // 8
   "AND",    // 9
   "NEQ",    // A
   "IOR",    // B
   "J/JS",   // C
   "MLT ",   // D - space because no register needed.
   "???",    // E
   "???"     // F
};


static const char* literalCmdSet [8] = {
   "SET",    // 0
   "ADD",    // 1
   "SUB",    // 2
   "CMP",    // 3
   "AND",    // 4
   "NEQ",    // 5
   "IOR",    // 6
   "???"     // 7 Shift and misc.
};

// bits 3-4
static const char* regName [4] = { "A", "R", "S", "T" };

// bits 5-6
static const char* indexName [4] = { "P", "R", "S", "T" };

// bits 5-6
static const char* compareJumpName [4] = { "JLT", "JGE", "JEQ", "JNE" };
static const char* triggerJumpName [4] = { "JVS", "JVN", "JCS", "JCN" };

static const char* regAValJumpName  [4] = { "JNGA", "JPZA", "JEZA", "JNZA" };
static const char* regRValJumpName  [4] = { "JNGR", "JPZR", "JEZR", "JNZR" };
static const char* regSValJumpName  [4] = { "JNGS", "JPZS", "JEZS", "JNZS" };

static const char* shiftSet [2] = { "SHL", "SHR" };

static const char* shiftIndex [3] = { "L", "A", "LC" };


//------------------------------------------------------------------------------
//
bool Diagnostics::isLoadReg (const Int16 instruction)
{
   return ((instruction & 0xE000) == 0x0000) ||   // 0xxx and 1xxx
          ((instruction & 0xE700) == 0xE000);     // E0xx, E8xx, F0xx F8xx
}

//------------------------------------------------------------------------------
//
bool Diagnostics::isCompare (const Int16 instruction)
{
   return ((instruction & 0xE000) == 0x6000) ||   // 6xxx and 7xxx
          ((instruction & 0xE700) == 0xE300);     // E3xx, EBxx, F3xx FBxx
}

//------------------------------------------------------------------------------
//
void Diagnostics::accessAddress (const Int16 addr)
{
   char instruction [20] = "NOOP";
   char strOffset [10] = "";

   const Int16 data = this->dataBus->getWord(addr);

   const int msb = (data >>  8) & 0xFF;
   const int lsb = data  & 0xFF;

   const int b0to3 = (data >> 12) & 15;
   const int b3to4 = (data >> 11) & 3;
   const int b5to6 = (data >>  9) & 3;
   const int b5to7 = (data >>  8) & 7;

   const int b4  = (data >> 11) & 1;
   const int b7  = (data >>  8) & 1;
   const int b15 = (data      ) & 1;

   const char* cmd = cmdSet  [b0to3];
   const char* reg = regName [b3to4];
   const char* idx = indexName [b5to6];
   const char* comma = ",";

   const int sign = (b7 == 0) ? +1 : -1;

   int offset = (b15 == 0) ? (sign * (data & 0xFF)) :
                             (sign * ((data >> 1) & 0x7F));
   const char* bytemode = (b15 == 1) ? "B" : "";
   const char* indirect = (b15 == 1) ? "I" : "";

   if (b0to3 < 12) {

      if (b0to3 >= 8) {
         // Only A and R registers
         reg = regName [b3to4 & 1];
      }

      if (b5to6 == 0) {
         snprintf (strOffset, sizeof (strOffset), ".%+d", offset+2);
      } else {
         snprintf (strOffset, sizeof (strOffset),  "%d", offset);
      }

      snprintf (instruction, sizeof (instruction), "%s%s %5s,%s%s",
                cmd, reg, strOffset, idx, bytemode);

   } else if (b0to3 == 12) {
       cmd = (data & 0x0800) == 0 ? "J " : "JS";
       offset = (sign * (data & 0xFE));

       if (b5to6 == 0) {
          snprintf (strOffset, sizeof (strOffset), ".%+d", offset+2);
          comma = (b15 == 1) ? "," : "";
          idx = "";  // No  ,P for jumps
       } else {
          snprintf (strOffset, sizeof (strOffset),  "%d", offset);
       }

       snprintf (instruction, sizeof (instruction), "%s   %5s%s%s%s",
                 cmd, strOffset, comma, idx, indirect);


   } else if ((b0to3 == 13) && (b4 == 0)) {
      // Conditional Jumps

      // JVN or JLT etc
      // Look at the prvious instructions.
      // Indicative, not perfect.
      //
      const Int16 prev = this->dataBus->getWord(addr - 2);
      if (this->isCompare(prev)) {
         cmd = compareJumpName [b5to6];

      } else if (this->isLoadReg(prev)) {
         const int prevReg = (prev >> 11) & 3;
         if      (prevReg == 0) { cmd = regAValJumpName [b5to6]; }
         else if (prevReg == 1) { cmd = regRValJumpName [b5to6]; }
         else if (prevReg == 2) { cmd = regSValJumpName [b5to6]; }
         else                   { cmd = triggerJumpName [b5to6]; }

      } else {
         cmd = triggerJumpName [b5to6];
      }
      snprintf (strOffset, sizeof (strOffset), ".%+d", offset+2);
      comma = (b15 == 1) ? "," : "";
      idx = "";  // No  ,P for jumps
      snprintf (instruction, sizeof (instruction), "%s  %5s%s%s%s",
                cmd, strOffset, comma, idx, indirect);


   } else if ((b0to3 == 13) && (b4 == 1)) {
      // MLT - multiply
      //
      reg = "";   // no reg - implicitly A
      if (b5to6 == 0) {
         snprintf (strOffset, sizeof (strOffset), ".%+d", offset+2);
      } else {
         snprintf (strOffset, sizeof (strOffset),  "%d", offset);
      }
      snprintf (instruction, sizeof (instruction), "%s%s %5s,%s%s",
                cmd, reg, strOffset, idx, bytemode);


   } else if (b5to7 < 7) {  // must be 14/15  Exxx/Fxxx and literal command.
      // Literals
      //
      cmd = literalCmdSet [b5to7];
      idx = "L";
      snprintf (instruction, sizeof (instruction), "%s%s %5d,L", cmd, reg, lsb);

   } else if ((data & 0xE7C0) == 0xE740) {
      // Shifts
      //
      reg = regName [(msb >> 3) & 3];
      cmd = shiftSet[(lsb >> 5) & 1];
      int mode = (lsb >> 4) & 1;
      int shift = lsb & 15;

      if (shift == 0 && mode ==1) {
          // shift 1,AC nor allowed.
      } else {
         if (shift == 0 && mode == 0) {
            // logical shift of 0 becomes  1,LC
            shift = 1;
            mode = 2;
         }

         snprintf (instruction, sizeof (instruction), "%s%s %5d,%s",
                   cmd, reg, shift, shiftIndex[mode]);
      }

   } else if ((data & 0xFF00) == 0xFF00) {
      // Miscellaneous
      //
      if (lsb < 4) {
         snprintf (instruction, sizeof (instruction), "SETL %5d", lsb);
      } else if (lsb == 0x20) {
         snprintf (instruction, sizeof (instruction), "CLRK");
      } else if (lsb == 0x21) {
         snprintf (instruction, sizeof (instruction), "SETK");
      } else if (lsb == 0xFF) {
         snprintf (instruction, sizeof (instruction), "NUL");
      }
   }

   // Add a little colour if/when this is a break point.
   //
   const bool ib = this->isBreakPoint(addr);
   const char* bp = ib ? "\033[33;1m*" : " ";
   const char* ap = ib ? "\033[00m"    : "";

   printf ("%s(%s)%s %s  %s\n", bp, hex(addr), ap, hex(data), instruction);
}

//------------------------------------------------------------------------------
//
void Diagnostics::accessAddress (const Int16 start, const Int16 finish)
{
   for (Int16 addr = start & 0xFFFE; addr < finish; addr += 2) {
      this->accessAddress (addr);
   }
}

//------------------------------------------------------------------------------
//
void Diagnostics::wideDump (const Int16 start, const Int16 finish)
{
   static const Int16 apl = 32;          // address per line
   static const Int16 mask = 0 - apl;    // line address mask

   if (finish <= start) return;

   const Int16 first = start & mask;                // round down
   const Int16 last  = (finish + apl - 1) & mask;   // round up

   for (Int16 base = first; base != last; base += apl) {
      printf ("(%s)", hex(base));

      for (Int16 offset = 0; offset < apl; offset += 2) {
         Int16 addr = base + offset;
         if ((addr >= start) && (addr < finish)) {
            printf (" %s", hex(this->dataBus->getWord(addr)));
         } else {
            printf("     ");
         }
      }

      printf("  |");

      for (Int16 offset = 0; offset < apl; offset += 1) {
         Int16 addr = base + offset;
         if ((addr >= start) && (addr < finish)) {
            char b = static_cast <char> (this->dataBus->getByte(addr));
            if (b < ' ' || b > 0x7e) b = '.';
            printf ("%c", b);
         } else {
            printf (" ");
         }
      }

      printf ("|\n");
   }
   printf ("\n");
}

//------------------------------------------------------------------------------
//
int Diagnostics::findBreak (const Int16 addr)
{
   int result = -1;

   for (int j = 0 ; j < this->breakCount; j++) {
      if (addr == this->breakList [j]) {
         result = j;
         break;
      }
   }

   return result;
}

//------------------------------------------------------------------------------
//
void Diagnostics::setBreak (const Int16 addr)
{
   int slot = this->findBreak(addr);
   if (slot >= 0) {
      printf ("break point already set at (%s)\n", hex(addr));
   } else if (this->breakCount >= ARRAY_LENGTH (this->breakList)) {
      printf ("!!!break table full, (%s) not set.\n", hex(addr));
   } else {
      this->breakList[this->breakCount++] = addr;
      printf ("break point set at (%s)\n", hex(addr));
   }
}

//------------------------------------------------------------------------------
//
void Diagnostics::clearBreak (const Int16 addr)
{
   int slot = this->findBreak(addr);
   if (slot >= 0) {
      if (slot == this->breakCount - 1) {
         this->breakCount--;
      } else {
         // shuffle up
         this->breakList[slot] = this->breakList[--this->breakCount];
      }
      printf ("break point at (%s) cleared\n", hex(addr));
   } else {
      printf ("no break point currently set at (%s)\n", hex(addr));
   }
}

//------------------------------------------------------------------------------
//
bool Diagnostics::isBreakPoint (const Int16 addr)
{
   return this->findBreak(addr) >= 0;
}

//------------------------------------------------------------------------------
//
void Diagnostics::listBreaks ()
{
   if (this->breakCount == 0) {
      printf ("None\n");
   } else {
      for (int j = 0 ; j < this->breakCount; j++) {
         printf ("%2d (%s)\n", j+1, hex(this->breakList [j]));
      }
   }
}

// end
