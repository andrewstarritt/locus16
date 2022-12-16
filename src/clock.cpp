/* clock.cpp
 *
 * Locus 16 Emulator clock module, part of the Locus 16 Emulator.
 *
 * Copyright (c) 2022 Anddrew Starritt
 *
 * The Locus 16 Emulator is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The Locus 16 Emulator is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Locus 16 Emulator.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "clock.h"

using namespace L16E;

//------------------------------------------------------------------------------
//
Clock::Clock (DataBus* const dataBus) :
   DataBus::Device (dataBus, 0x7C00, 0x7C04, "Clock", false)
{
   this->isRunning = false;
   this->interval = 0;
   this->countDown = 0.0;
   this->interruptPending = false;
   this->numberActiveDevices = 1;
}

//------------------------------------------------------------------------------
//
Clock::~Clock() { }   // place holder

//------------------------------------------------------------------------------
//
void Clock::executeCycle()
{
   if (this->isRunning) {
      // A typlical ALP instruction is 2.25 uSec
      // If more than one active device, we should adjust this.
      // Note: it is far from linear, due to bus contention
      //
      double duration = (3.0 * 2.25) / (this->numberActiveDevices + 2.0);
      this->countDown -= duration;
      if (this->countDown <= 0.0) {
         this->interruptPending = true;
         // reset count down (convert interval from mS to uS).
         this->countDown += 1000.0 * this->interval;
         if (this->countDown < 10) this->countDown = 10;
      }
   }
}

//------------------------------------------------------------------------------
//
void Clock::setNumberActiveDevices(const int n)
{
   this->numberActiveDevices = MAX(1, n);
}

//------------------------------------------------------------------------------
//
bool Clock::testAndClearInterruptPending ()
{
   bool result = this->interruptPending;
   this->interruptPending = false;
   return result;
}

//------------------------------------------------------------------------------
//
Int16 Clock::getWord(const Int16 addr) const
{
   Int16 result;

   if (addr == 0x7C00) {
      result = this->isRunning ? 1 : 0;
   } else if (addr == 0x7C02) {
      result = Int16 (this->interval);
   } else {
      result = 0xFFFF;
   }

   return result;
}

//------------------------------------------------------------------------------
//
void Clock::setWord(const Int16 addr, const Int16 value)
{
   if (addr == 0x7C00) {
      this->isRunning = value & 1;
   } else if (addr == 0x7C02) {
      // Ensure we treat as unsigned when we detrmine the interval.
      //
      this->interval = (value >= 0) ? value : value + 0x010000;
      this->countDown = 1000.0 * this->interval;  // reset count down (mS => uS)
      if (this->countDown < 10) this->countDown = 10;
   }
}

// end

