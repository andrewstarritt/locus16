/* clock.h
 *
 * Clock module, part of the Locus 16 Emulator.
 *
 * Copyright (c) 2022  Andrew C. Starritt
 *
 * The Locus 16 Emulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * The Locus 16 Emulator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License and
 * the Lesser GNU General Public License along with the Locus 16 Emulator.
 * If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * andrew.starritt@gmail.com
 * PO Box 3118, Prahran East, Victoria 3181, Australia.
 */

#ifndef L16E_CLOCK_H
#define L16E_CLOCK_H

#include "data_bus.h"

namespace L16E {

// This emulates a clock.
//
class Clock : public DataBus::Device {
public:
   // Clock address is 0x7C00 to 0x7C04
   // 0x7C00 status/control register
   // bit 0: write 0 stop clock
   // bit 0: write 1 to start clock
   // bit 0; read to find status - running(1) or stopped (0)
   // 0x7C02 interrupt interval (in simulated mSec based on 2.25 uSec/instruction)
   // Value written treated as an unsigned 16 bit integer.
   //
   explicit Clock (DataBus* const dataBus);
   virtual ~Clock();

   void setNumberActiveDevices(const int n);
   bool testAndClearInterruptPending ();

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16  value);

   void executeCycle();   // called prior to each instruction execution

private:
   int numberActiveDevices;
   bool isRunning;
   Int16 interval;      // in emulated mSec
   double countDown;    // in emulated uSec
   volatile bool interruptPending;
};

}

#endif // L16E_CLOCK_H
