/* serial.h
 *
 * Serial module, part of the Locus 16 Emulator.
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

#ifndef L16E_SERIAL_H
#define L16E_SERIAL_H

#include "peripheral.h"
#include "data_bus.h"

namespace L16E {

// This emulates a serial channel.
//
class Serial : public DataBus::Device {
public:

   enum Type {
      Input = 0,  // input from an external peripheral.
      Output      // output to an external peripheral.
   };

   // statusRegister - device is ready to read/write when (content & =XF000) is =XC000
   // dataRegister   - read byte/written byte is in least significant byte of register.
   // If the status register is, for example, =X7B10, then the
   // data register is implicitly =X7B12.
   //
   explicit Serial (const Type type,
                    const Int16 statusRegisterAddress,
                    DataBus* const dataBus);
   virtual ~Serial();

   void connect (Peripheral* peripheral);

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16  value);

private:
   const Type type;
   const Int16 statusRegisterAddress;
   const Int16 dataRegisterAddress;
   Peripheral* peripheral;

   mutable bool bufferedByteExists;
   mutable UInt8 bufferedByte;
};

}

#endif // L16E_SERIAL_H
