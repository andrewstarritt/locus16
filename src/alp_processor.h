/* alp_processor.h
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#ifndef L16E_ALP_PROCESSOR_H
#define L16E_ALP_PROCESSOR_H

#include <string.h>
#include "data_bus.h"

namespace L16E {

class ALP_Processor : public DataBus::ActiveDevice
{
public:
   enum ALPKinds {
      alp1,
      alp2       // not implemented yet
   };

   explicit ALP_Processor(const int slot,           // 1 for primary etc.
                          const ALPKinds alpKind,   // 1 or 2
                          DataBus* const dataBus);
   ~ALP_Processor();

   void requestInterrupt ();
   bool execute();   // fetch and execute one instruction

   unsigned int getLevel() const;
   void dumpRegisters(const unsigned int level) const;
   void dumpRegisters() const;
   Int16 getPreg() const;  // of current level

   // Register access
   //
   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16  value);

private:
   const int slot;
   const ALPKinds alpKind;
   const unsigned int numberLevels;

   unsigned int level;       // 0 .. 3 ALP1,  0 .. 1  ALP2/3
   Int16 preg [4];
   Int16 areg [4];
   Int16 rreg [4];
   Int16 sreg [4];
   Int16 treg [4];
   bool cTrigger [4];        // carry flag
   bool vTrigger [4];        // overflow flag
   bool kFlag [4];           // inhibits interrupts
   bool interruptRequested;  // indicates an interrupt is pending.

   bool debug;
};

}

#endif // L16E_ALP_PROCESSOR_H
