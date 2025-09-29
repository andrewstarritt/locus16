/* serial.h
 *
 * Serial module, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
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
