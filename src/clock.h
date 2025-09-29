/* clock.h
 *
 * Clock module, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
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
