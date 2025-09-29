/* diagnostics.h
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 */

#ifndef L16E_DIAGNOSTICS_H
#define L16E_DIAGNOSTICS_H

#include "data_bus.h"

namespace L16E {

class Diagnostics {
public:
   explicit Diagnostics (DataBus* dataBus);
   ~Diagnostics ();

   void wideDump (const Int16 start, const Int16 finish);

   void accessAddress (const Int16 addr);
   void accessAddress (const Int16 start, const Int16 finish);

   void setBreak (const Int16 addr);
   void clearBreak (const Int16 addr);
   bool isBreakPoint (const Int16 addr);
   void listBreaks ();

private:
   static char* hex (const Int16 x);
   bool isLoadReg (const Int16 instruction);
   bool isCompare (const Int16 instruction);
   int findBreak (const Int16 addr);  // returns slot (0..39) or -1

   DataBus* const dataBus;   // ptr constant, not what is pointed to

   int breakCount;
   Int16 breakList [40];
};

}

#endif  // L16E_DIAGNOSTICS_H
