/* alp_processor.cpp
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "alp_processor.h"

#include <stdio.h>
#include <string.h>

// NOTE: All these macros all expect a local int variable called useLevel
//
#define PREG this->preg [useLevel]
#define AREG this->areg [useLevel]
#define RREG this->rreg [useLevel]
#define SREG this->sreg [useLevel]
#define TREG this->treg [useLevel]
#define CTRG this->cTrigger[useLevel]
#define VTRG this->vTrigger[useLevel]
#define KFLG this->kFlag[useLevel]

#define SETP(value) this->preg [useLevel] = value
#define SETA(value) this->areg [useLevel] = value
#define SETR(value) this->rreg [useLevel] = value
#define SETS(value) this->sreg [useLevel] = value
#define SETT(value) this->treg [useLevel] = value

// The primary ALP hardware mapped address range is =X7F00 to =X7FFF inclusive.
// For a DataBus::Device we specify inclusive lower address and exclusive upper
// address, so we would like to specify +32768. However this is beyond the allowed
// range of a 16 bit signed integer, so we go one less; and this means we cannot
// address the byte at =X7FFF. No real problem here as nothing of interest at
// =X7FFF as I recall and ALP registers are addressed by word anyway.
//
// Primary ALP   is 0x7F00 to 0x7FFF
// Secondary ALP is 0x7E00 to 0x7EFF
// Tertiary ALP  is 0x7D00 to 0x7DFF  - if we were to allow three
//
#define ADDR_LOW(n)  (0x7F00 - (n-1)*0x0100)
#define ADDR_HIGH(n) (ADDR_LOW(n) + 0x00FF)    // must avoid the over flow

using namespace L16E;

//------------------------------------------------------------------------------
//
static char* nameOf (const L16E::ALP_Processor::ALPKinds akind,
                     int instance)
{
   char buffer [20];
   int kind = 1;
   if (akind == ALP_Processor::alp2) kind = 2;
   snprintf(buffer, sizeof (buffer), "ALP%d Processor (%d)", kind, instance);
   return strndup (buffer, sizeof (buffer));
}

//------------------------------------------------------------------------------
//
ALP_Processor::ALP_Processor(const int slotIn,       // 1 for primary etc.
                             const ALPKinds alpKindIn,
                             DataBus* const dataBus) :
   DataBus::ActiveDevice (dataBus, ADDR_LOW(slotIn), ADDR_HIGH(slotIn),
                          nameOf (alpKindIn, slotIn)),
   slot (slotIn),
   alpKind (alpKindIn),
   numberLevels (alpKindIn == alp1 ? 4 : 2),
   debug (false)
{
   if ((this->slot != 1) && (this->slot != 2)) {
      fprintf (stderr, "Bad slot: %d\n", slotIn);
      return;
   }

   for (int useLevel = 0; useLevel < 4; useLevel++) {
      this->preg [useLevel] = 0;
      this->areg [useLevel] = 0;
      this->rreg [useLevel] = 0;
      this->sreg [useLevel] = 0;
      this->treg [useLevel] = 0;

      this->cTrigger [useLevel] = false;
      this->vTrigger [useLevel] = false;
      this->kFlag [useLevel] = false;
   }

   const unsigned int useLevel = 1;
   this->level = useLevel;
   this->interruptRequested = false;

   SETP(DataBus::addressFirst);    // =X8000
}

//------------------------------------------------------------------------------
//
ALP_Processor::~ALP_Processor() { }

//------------------------------------------------------------------------------
//
unsigned int ALP_Processor::getLevel() const
{
   return this->level;
}

//------------------------------------------------------------------------------
//
void ALP_Processor::dumpRegisters(const unsigned int useLevel) const
{
   // Many macros assume level variable is called "useLevel" exists".

   if (useLevel >= this->numberLevels) return;

   printf ("%d: Level %d: ", this->slot, useLevel);

   // Need to mask these values with 0xFFFF to make "positive" for printf.
   //
   printf ("P: %04X  ", PREG & 0xFFFF);
   printf ("A: %04X  ", AREG & 0xFFFF);
   printf ("R: %04X  ", RREG & 0xFFFF);
   printf ("S: %04X  ", SREG & 0xFFFF);
   printf ("T: %04X  ", TREG & 0xFFFF);
   printf ("C: %s  ",   CTRG ? "1" : "0");
   printf ("V: %s  ",   VTRG ? "1" : "0");
   printf ("K: %s\n",   KFLG ? "1" : "0");
}

//------------------------------------------------------------------------------
//
void ALP_Processor::dumpRegisters () const
{
   this->dumpRegisters(this->level);
}

//------------------------------------------------------------------------------
//
Int16  ALP_Processor::getPreg() const
{
   const unsigned int useLevel = this->level;
   return PREG;
}

//------------------------------------------------------------------------------
// Register access - ALP registers are mempry mapped.
//
// Address to register mapping is very speculative.
// Pretty sure P0 is at =X7F02 for primary ALP.
//
Int16 ALP_Processor::getWord(const Int16 addr) const
{
   const unsigned int alpAddr = addr & 0x00FF;
   const unsigned int useLevel = alpAddr >> 4;

   if (alpAddr == 0) return (this->interruptRequested << 4) | this->level;
   if (useLevel >= this->numberLevels) return DataBus::allOnes;

   const unsigned int reg = addr & 0x000F;
   switch (reg) {
      case 0x02: return PREG; break;
      case 0x04: return AREG; break;
      case 0x06: return RREG; break;
      case 0x08: return SREG; break;
      case 0x0A: return TREG; break;
      case 0x0C: {
            Int16 r = (CTRG << 2) | (VTRG << 1) | (KFLG);
            return r;
            break;
         }
      default:   return DataBus::allOnes; break;
   }
}

//------------------------------------------------------------------------------
//
void ALP_Processor::setWord(const Int16 addr, const Int16 value)
{
   const unsigned int useLevel = (addr >> 4) & 0x000F;
   if (useLevel >= this->numberLevels) return;
   const unsigned int reg = addr & 0x000F;
   switch (reg) {
      case 0x02: SETP(value); break;
      case 0x04: SETA(value); break;
      case 0x06: SETR(value); break;
      case 0x08: SETS(value); break;
      case 0x0A: SETT(value); break;
      case 0x0C: {
            CTRG = (value & 4) == 4;
            VTRG = (value & 2) == 2;
            KFLG = (value & 1) == 1;
            break;
         }
      default:   break;
   }
}

//------------------------------------------------------------------------------
//
void ALP_Processor::requestInterrupt()
{
   this->interruptRequested = true;
}

//------------------------------------------------------------------------------
//
bool ALP_Processor::execute()
{
   // Sanity checks
   //
   if ((this->slot != 1) && (this->slot != 2)) {
      printf ("Unexpected processor slot: %d\n", this->slot);
      return false;
   }

   if (this->level >= this->numberLevels) {
      printf ("Unexpected process level: %u\n", this->level);
      return false;
   }

   // First check for a pending interrupt request.
   // Only level 0 can get interrupted - it was a design error.
   // Note: we can't use KFLG until useLevel declared.
   //
   if (this->interruptRequested && (this->level == 0) && !this->kFlag[this->level]) {
      // Switch to level 1.
      //
      this->level = 1;
      this->interruptRequested = false;    // clear the request
   }

   const int useLevel = this->level;       // Many macros assume useLevel exists.
   const Int16 address = PREG;
   const Int16 instruction = this->dataBus->getWord(address);   // Fetch

   // Update P first-thing before executing the instruction proper.
   //
   SETP(address + 2);

   const UInt8 msiByte = (instruction >> 8) & 255;
   const UInt8 lsiByte = instruction & 255;

   // We calc a few things even if not all needed
   // We will worry about efficiency later.
   //
   const bool isWord = (lsiByte & 1) == 0;           // as opposed to isByte
   const bool isIndirect = (lsiByte & 1) == 1;       // as opposed to direct
   const Int16 sign = (msiByte & 1) == 0 ? +1 : -1;  // offset sign
   const Int16 wordOffset = sign * lsiByte;
   const Int16 byteOffset = sign * (lsiByte >> 1);
   const Int16 jumpOffset = sign * (lsiByte & 0xFE);

   #define WOFFSET   lsInstructionByte
   #define BOFFSET  (lsInstructionByte >> 1)
   #define IOFFSET  (lsInstructionByte & 0xFE)

   // Macro funtions
   //
   #define UNDEFINED {                                                        \
      printf ("Undefined instruction: (%04X) %04X\n",                         \
              address & 0xFFFF, instruction & 0xFFFF);                        \
      return false;                                                           \
   }


   #define STRX_Y(x,y)  {                                                     \
      if (isWord) {                                                           \
         this->dataBus->setWord (y##REG + wordOffset, x##REG);                \
      } else {                                                                \
         this->dataBus->setByte (y##REG + byteOffset, x##REG);                \
      }                                                                       \
   }


   // Basic operations
   // Trigger association is kind of speculitive.
   //
   #define SETX(x, regvalue) {                                                \
      SET##x(regvalue);                                                       \
      const int r = x##REG;                                                   \
      CTRG = (r == 0);                                                        \
      VTRG = (r <  0);                                                        \
   }


   #define ADDX(x, operand) {                                                 \
      const int t = x##REG + (operand);                                       \
      SET##x(t);                                                              \
      CTRG = ((t >> 16) & 1) == 1;                                            \
      VTRG = (t > 32767) || (t < -32768);                                     \
   }


   #define SUBX(x, operand) {                                                 \
      const int t = x##REG - (operand);                                       \
      SET##x(t);                                                              \
      CTRG = ((t >> 16) & 1) == 1;                                            \
      VTRG = (t > 32767) || (t < -32768);                                     \
   }


   // TODO: verify which JxC/JxN corresponds to == and <
   //
   #define CMPX(x, operand) {                                                 \
      const int r = x##REG;                                                   \
      const int v = operand;                                                  \
      CTRG = (r == v);                                                        \
      VTRG = (r <  v);                                                        \
   }


   #define ANDX(x, operand)  SET##x(x##REG & (operand))

   #define IORX(x, operand)  SET##x(x##REG | (operand))

   #define NEQX(x, operand)  SET##x(x##REG ^ (operand))


   #define ACCESS(y) (isWord ? this->dataBus->getWord(y##REG + wordOffset)    \
                             : this->dataBus->getByte(y##REG + byteOffset))


   #define SETX_Y(x,y)  SETX(x, ACCESS(y))
   #define SETT_Y(  y)  SETT(   ACCESS(y))
   #define ADDX_Y(x,y)  ADDX(x, ACCESS(y))
   #define SUBX_Y(x,y)  SUBX(x, ACCESS(y))
   #define CMPX_Y(x,y)  CMPX(x, ACCESS(y))
   #define ANDX_Y(x,y)  ANDX(x, ACCESS(y))
   #define IORX_Y(x,y)  IORX(x, ACCESS(y))
   #define NEQX_Y(x,y)  NEQX(x, ACCESS(y))


   #define MLTA_Y(y) {                                                        \
      long t = AREG * ACCESS(y) * 2;                                          \
      SETA(t >> 16);                                                          \
      SETR(t);                                                                \
   }


   // General Jump macro
   //
   #define JUMP(condition, index) {                                           \
      if (condition) {                                                        \
         if (isIndirect) {                                                    \
            SETP(this->dataBus->getWord(index##REG + jumpOffset));            \
         } else {                                                             \
            SETP(index##REG + jumpOffset);                                    \
         }                                                                    \
      }                                                                       \
   }


   // These are not independent instructions.
   // #define JEZX(x)      JUMP(x##REG == 0, P)
   // #define JNZX(x)      JUMP(x##REG != 0, P)
   // #define JPZX(x)      JUMP(x##REG >= 0, P)
   // #define JNGX(x)      JUMP(x##REG <  0, P)


   if (this->debug) {
      printf ("%+1d   B:%d   I:%d\n", sign, !isWord, isIndirect);
      printf ("%+3d   %+3d   %+3d \n", wordOffset, byteOffset, jumpOffset);
   }

   // Decode/execute the instruction
   //
   switch (msiByte) {

      // SET i.e. LOAD
      //
      case 0x00:
      case 0x01:
         SETX_Y(A, P);
         break;

      case 0x02:
      case 0x03:
         SETX_Y(A, R);
         break;

      case 0x04:
      case 0x05:
         SETX_Y(A, S);
         break;

      case 0x06:
      case 0x07:
         SETX_Y(A, T);
         break;

      case 0x08:
      case 0x09:
         SETX_Y(R, P);
         break;

      case 0x0A:
      case 0x0B:
         SETX_Y(R, R);
         break;

      case 0x0C:
      case 0x0D:
         SETX_Y(R, S);
         break;

      case 0x0E:
      case 0x0F:
         SETX_Y(R, T);
         break;

      case 0x10:
      case 0x11:
         SETX_Y(S, P);
         break;

      case 0x12:
      case 0x13:
         SETX_Y(S, R);
         break;

      case 0x14:
      case 0x15:
         SETX_Y(S, S);
         break;

      case 0x16:
      case 0x17:
         SETX_Y(S, T);
         break;

      case 0x18:
      case 0x19:
         SETT_Y(P);
         break;

      case 0x1A:
      case 0x1B:
         SETT_Y(R);
         break;

      case 0x1C:
      case 0x1D:
         SETT_Y(S);
         break;

      case 0x1E:
      case 0x1F:
         SETT_Y(T);
         break;

      // STR
      //
      case 0x20:
      case 0x21:
         STRX_Y(A, P);
         break;

      case 0x22:
      case 0x23:
         STRX_Y(A, R);
         break;

      case 0x24:
      case 0x25:
         STRX_Y(A, S);
         break;

      case 0x26:
      case 0x27:
         STRX_Y(A, T);
         break;

      case 0x28:
      case 0x29:
         STRX_Y(R, P);
         break;

      case 0x2A:
      case 0x2B:
         STRX_Y(R, R);
         break;

      case 0x2C:
      case 0x2D:
         STRX_Y(R, S);
         break;

      case 0x2E:
      case 0x2F:
         STRX_Y(R, T);
         break;

      case 0x30:
      case 0x31:
         STRX_Y(S, P);
         break;

      case 0x32:
      case 0x33:
         STRX_Y(S, R);
         break;

      case 0x34:
      case 0x35:
         STRX_Y(S, S);
         break;

      case 0x36:
      case 0x37:
         STRX_Y(S, T);
         break;

      case 0x38:
      case 0x39:
         STRX_Y(T, P);
         break;

      case 0x3A:
      case 0x3B:
         STRX_Y(T, R);
         break;

      case 0x3C:
      case 0x3D:
         STRX_Y(T, S);
         break;

      case 0x3E:
      case 0x3F:
         STRX_Y(T, T);
         break;


      // ADD
      //
      case 0x40:
      case 0x41:
         ADDX_Y(A, P);
         break;

      case 0x42:
      case 0x43:
         ADDX_Y(A, R);
         break;

      case 0x44:
      case 0x45:
         ADDX_Y(A, S);
         break;

      case 0x46:
      case 0x47:
         ADDX_Y(A, T);
         break;

      case 0x48:
      case 0x49:
         ADDX_Y(R, P);
         break;

      case 0x4A:
      case 0x4B:
         ADDX_Y(R, R);
         break;

      case 0x4C:
      case 0x4D:
         ADDX_Y(R, S);
         break;

      case 0x4E:
      case 0x4F:
         ADDX_Y(R, T);
         break;

      case 0x50:
      case 0x51:
         ADDX_Y(S, P);
         break;

      case 0x52:
      case 0x53:
         ADDX_Y(S, R);
         break;

      case 0x54:
      case 0x55:
         ADDX_Y(S, S);
         break;

      case 0x56:
      case 0x57:
         ADDX_Y(S, T);
         break;

      case 0x58:
      case 0x59:
         ADDX_Y(T, P);
         break;

      case 0x5A:
      case 0x5B:
         ADDX_Y(T, R);
         break;

      case 0x5C:
      case 0x5D:
         ADDX_Y(T, S);
         break;

      case 0x5E:
      case 0x5F:
         ADDX_Y(T, T);
         break;

      // CMP
      //
      case 0x60:
      case 0x61:
         CMPX_Y(A, P);
         break;

      case 0x62:
      case 0x63:
         CMPX_Y(A, R);
         break;

      case 0x64:
      case 0x65:
         CMPX_Y(A, S);
         break;

      case 0x66:
      case 0x67:
         CMPX_Y(A, T);
         break;

      case 0x68:
      case 0x69:
         CMPX_Y(R, P);
         break;

      case 0x6A:
      case 0x6B:
         CMPX_Y(R, R);
         break;

      case 0x6C:
      case 0x6D:
         CMPX_Y(R, S);
         break;

      case 0x6E:
      case 0x6F:
         CMPX_Y(R, T);
         break;

      case 0x70:
      case 0x71:
         CMPX_Y(S, P);
         break;

      case 0x72:
      case 0x73:
         CMPX_Y(S, R);
         break;

      case 0x74:
      case 0x75:
         CMPX_Y(S, S);
         break;

      case 0x76:
      case 0x77:
         CMPX_Y(S, T);
         break;

      case 0x78:
      case 0x79:
         CMPX_Y(T, P);
         break;

      case 0x7A:
      case 0x7B:
         CMPX_Y(T, R);
         break;

      case 0x7C:
      case 0x7D:
         CMPX_Y(T, S);
         break;

      case 0x7E:
      case 0x7F:
         CMPX_Y(T, T);
         break;

      // SUB
      //
      case 0x80:
      case 0x81:
         SUBX_Y(A, P);
         break;

      case 0x82:
      case 0x83:
         SUBX_Y(A, R);
         break;

      case 0x84:
      case 0x85:
         SUBX_Y(A, S);
         break;

      case 0x86:
      case 0x87:
         SUBX_Y(A, T);
         break;

      case 0x88:
      case 0x89:
         SUBX_Y(R, P);
         break;

      case 0x8A:
      case 0x8B:
         SUBX_Y(R, R);
         break;

      case 0x8C:
      case 0x8D:
         SUBX_Y(R, S);
         break;

      case 0x8E:
      case 0x8F:
         SUBX_Y(R, T);
         break;

      // AND/MASK
      //
      case 0x90:
      case 0x91:
         ANDX_Y(A, P);
         break;

      case 0x92:
      case 0x93:
         ANDX_Y(A, R);
         break;

      case 0x94:
      case 0x95:
         ANDX_Y(A, S);
         break;

      case 0x96:
      case 0x97:
         ANDX_Y(A, T);
         break;

      case 0x98:
      case 0x99:
         ANDX_Y(R, P);
         break;

      case 0x9A:
      case 0x9B:
         ANDX_Y(R, R);
         break;

      case 0x9C:
      case 0x9D:
         ANDX_Y(R, S);
         break;

      case 0x9E:
      case 0x9F:
         ANDX_Y(R, T);
         break;


      // NEQ/XOR
      //
      case 0xA0:
      case 0xA1:
         NEQX_Y(A, P);
         break;

      case 0xA2:
      case 0xA3:
         NEQX_Y(A, R);
         break;

      case 0xA4:
      case 0xA5:
         NEQX_Y(A, S);
         break;

      case 0xA6:
      case 0xA7:
         NEQX_Y(A, T);
         break;

      case 0xA8:
      case 0xA9:
         NEQX_Y(R, P);
         break;

      case 0xAA:
      case 0xAB:
         NEQX_Y(R, R);
         break;

      case 0xAC:
      case 0xAD:
         NEQX_Y(R, S);
         break;

      case 0xAE:
      case 0xAF:
         NEQX_Y(R, T);
         break;


      // IOR
      //
      case 0xB0:
      case 0xB1:
         IORX_Y(A, P);
         break;

      case 0xB2:
      case 0xB3:
         IORX_Y(A, R);
         break;

      case 0xB4:
      case 0xB5:
         IORX_Y(A, S);
         break;

      case 0xB6:
      case 0xB7:
         IORX_Y(A, T);
         break;

      case 0xB8:
      case 0xB9:
         IORX_Y(R, P);
         break;

      case 0xBA:
      case 0xBB:
         IORX_Y(R, R);
         break;

      case 0xBC:
      case 0xBD:
         IORX_Y(R, S);
         break;

      case 0xBE:
      case 0xBF:
         IORX_Y(R, T);
         break;


      // J
      //
      case 0xC0:
      case 0xC1:
         JUMP(true, P);
         break;

      case 0xC2:
      case 0xC3:
         JUMP(true, R);
         break;

      case 0xC4:
      case 0xC5:
         JUMP(true, S);
         break;

      case 0xC6:
      case 0xC7:
         JUMP(true, T);
         break;


      // JS
      //
      case 0xC8:
      case 0xC9:
         JUMP(true, P);
         SETS(address + 2);
         break;

      case 0xCA:
      case 0xCB:
         JUMP(true, R);
         SETS(address + 2);
         break;

      case 0xCC:
      case 0xCD:
         JUMP(true, S);
         SETS(address + 2);
         break;

      case 0xCE:
      case 0xCF:
         JUMP(true, T);
         SETS(address + 2);
         break;


      // JVS/JLT
      //
      case 0xD0:
      case 0xD1:
         JUMP(VTRG,  P);
         break;

      // JVN/JGE
      //
      case 0xD2:
      case 0xD3:
         JUMP(!VTRG, P);
         break;

      // JCS/JEQ
      //
      case 0xD4:
      case 0xD5:
         JUMP(CTRG,  P);
         break;

      // JCN/JNE
      //
      case 0xD6:
      case 0xD7:
         JUMP(!CTRG, P);
         break;


      // MLT
      //
      case 0xD8:
      case 0xD9:
         MLTA_Y(P);
         break;

      case 0xDA:
      case 0xDB:
         MLTA_Y(R);
         break;

      case 0xDC:
      case 0xDD:
         MLTA_Y(S);
         break;

      case 0xDE:
      case 0xDF:
         MLTA_Y(T);
         break;


      // LITerals
      //
      case 0xE0:
         SETX(A, lsiByte);
         break;

      case 0xE1:
         ADDX(A, lsiByte);
         break;

      case 0xE2:
         SUBX(A, lsiByte);
         break;

      case 0xE3:
         CMPX(A, lsiByte);
         break;

      case 0xE4:
         ANDX(A, lsiByte);
         break;

      case 0xE5:
         NEQX(A, lsiByte);
         break;

      case 0xE6:
         IORX(A, lsiByte);
         break;

      // case 0xE7 - see below

      case 0xE8:
         SETX(R, lsiByte);
         break;

      case 0xE9:
         ADDX(R, lsiByte);
         break;

      case 0xEA:
         SUBX(R, lsiByte);
         break;

      case 0xEB:
         CMPX(R, lsiByte);
         break;

      case 0xEC:
         ANDX(R, lsiByte);
         break;

      case 0xED:
         NEQX(R, lsiByte);
         break;

      case 0xEE:
         IORX(R, lsiByte);
         break;

      // case 0xEF - see below

      case 0xF0:
         SETX(S, lsiByte);
         break;

      case 0xF1:
         ADDX(S, lsiByte);
         break;

      case 0xF2:
         SUBX(S, lsiByte);
         break;

      case 0xF3:
         CMPX(S, lsiByte);
         break;

      case 0xF4:
         ANDX(S, lsiByte);
         break;

      case 0xF5:
         NEQX(S, lsiByte);
         break;

      case 0xF6:
         IORX(S, lsiByte);
         break;

      // case 0xF7 - see below

      case 0xF8:
         // For T we go direct - no trigger setting
         SETT(lsiByte);
         break;

      case 0xF9:
         ADDX(T, lsiByte);
         break;

      case 0xFA:
         SUBX(T, lsiByte);
         break;

      case 0xFB:
         CMPX(T, lsiByte);
         break;

      case 0xFC:
         ANDX(T, lsiByte);
         break;

      case 0xFD:
         NEQX(T, lsiByte);
         break;

      case 0xFE:
         IORX(T, lsiByte);
         break;

      case 0xE7:  // Shifts
      case 0xEF:  // Shifts
      case 0xF7:  // Shifts
      case 0xFF:  // Shifts and specials.
         {
            if ((lsiByte & 0xC0) == 0x40) {
               // This is a shift
               // All verfy speculative
               //
               enum Dirn { left = 0, right = 1};
               enum Mode { logical = 0, arithmetic = 1 };

               const bool cTrigWasSet = CTRG;

               const int reg = (msiByte >> 3) & 3;
               const Dirn leftRight         = Dirn ((lsiByte >> 5) & 1);
               const Mode logicalArithmetic = Mode ((lsiByte >> 4) & 1);

               int shift = lsiByte & 0x0F;
               bool coupled = false;

               if (shift == 0) {
                  if (logicalArithmetic == arithmetic) {
                     // shift 0 impiles 1,LC ; 1,AC not allowed
                     UNDEFINED;
                     break;
                  }
                  coupled = 1;
                  shift = 1;
               }

               Int16 regValue;
               switch (reg) {
                  case 0: regValue = AREG; break;
                  case 1: regValue = RREG; break;
                  case 2: regValue = SREG; break;
                  case 3: regValue = TREG; break;
               }

               if (leftRight == left) {
                  regValue = regValue << (shift - 1);
                  CTRG = (regValue & 0x8000) == 0x8000;  // Extract last bit to be shifted.
                  regValue = regValue << 1;
                  if (coupled && cTrigWasSet) {
                     regValue |= 0x0001;
                  }
               } else {
                  regValue = regValue >> (shift - 1);    // naturally arithmetic
                  CTRG = (regValue & 0x0001) == 0x0001;  // Extract last bit to be shifted.
                  regValue = regValue >> 1;
                  if (coupled && cTrigWasSet) {
                     regValue |= 0x8000;
                  }

                  if (logicalArithmetic == logical) {
                     unsigned long mask = 0x0000FFFF >> shift;
                     regValue = regValue & mask;
                  }
               }

               switch (reg) {
                  case 0: SETA(regValue); break;
                  case 1: SETR(regValue); break;
                  case 2: SETS(regValue); break;
                  case 3: SETT(regValue); break;
               }

               break;
            }

            // Check other specials
            //
            if (msiByte == 0xFF) {
               // ALP1 has 4 levels, ALP2 has two levels
               if (lsiByte < this->numberLevels) {
                  // SETL  XX
                  this->level = lsiByte;

               } else if (lsiByte == 0x20) {
                  // CLRK
                  KFLG = false;

               } else if (lsiByte == 0x21) {
                  // SETK
                  KFLG = true;

               } else if (lsiByte == 0xFF) {
                  // NUL

               } else {
                  UNDEFINED;
               }
            }
         }
         break;

      default:
         UNDEFINED;
         break;
   }

   return true;
}

// end
