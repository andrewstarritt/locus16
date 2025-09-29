/* terminal.h
 *
 * Terminal module, part of the Locus 16 Emulator.
 * Requires two serial devices.
 *
 * Copyright (c) 2022-2025  Andrew C. Starritt
 *
 * The Locus 16 Emulator is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License.
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
 */

#ifndef L16E_TERMINAL_H
#define L16E_TERMINAL_H

#include "locus16_common.h"
#include "peripheral.h"

namespace L16E {

class Terminal : public Peripheral
{
public:
   explicit Terminal ();
   virtual ~Terminal();

   // Initialise the terminal device.
   //
   bool initialise();

   bool readByte(UInt8& value);
   bool writeByte(const UInt8 value);

private:
   void clear();

   int pt_fd;    // pseudo-terminal device
   int xt_fd;    // xterm device
   const char* ptname;
   int xterm_pid;
};

}

#endif // L16E_TERMINAL_H
