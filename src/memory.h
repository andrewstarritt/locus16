/* memory.h
 *
 * Creates extended RAM memory addressed from 0x9000 to 0x7000
 * together with a memory controller, part of the Locus 16 Emulator
 * application.
 *
 * Copyright (c) 2021-2022  Andrew C. Starritt
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

#ifndef L16E_MEMORY_H
#define L16E_MEMORY_H

#include "data_bus.h"

namespace L16E {

class MemoryMapper : public DataBus::Device {
public:
   enum Constants {
      maximumNumberOfMaps = 8
   };

   // Controls extended memory access - upto 296 kBytes.
   // Map control register addresses are (arbitarily choosen):
   // 0x7B00
   // 0x7B02
   // ...
   // 0x7B0E
   //
   // Default map values are 0x0000
   //
   // MS Nibble 1 maps 0x2000 .. 0x2FFF
   //    Nibble 2 maps 0x3000 .. 0x3FFF
   //    Nibble 3 maps 0x4000 .. 0x4FFF
   // LS Nibble 4 maps 0x5000 .. 0x5FFF
   // Each nibble contains one of 16 values.
   //
   explicit MemoryMapper (DataBus* const dataBus);
   virtual ~MemoryMapper();

   void setActiveIdentity(const int id);

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16  value);

private:
   friend class Memory;

   // Maps 16 bit address into "physical" memory address
   //
   int mapAddress (const Int16 addr) const;

   // Do we need a map word for each device instance, say if two ALPs.
   // Likewise 2 or more pre calculated offsets.
   //
   Int16 mapValues [maximumNumberOfMaps];

   int offsets [maximumNumberOfMaps][16];
   int activeIdentity;
};


class Memory : public DataBus::Device {
public:
   // Note: We create a single memory device.
   // An actual locus would have multiple memory devices/cards.
   //
   explicit Memory (const int number,
                    MemoryMapper* controllerIn,
                    DataBus* const dataBus);
   virtual ~Memory();

   // Initialise all memory to the specified value.
   //
   void initialise(const Int16 value);

   UInt8 getByte(const Int16 addr) const;
   void setByte(const Int16 addr, const UInt8 value);

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16  value);

private:
   const int number;
   MemoryMapper* const controller;   // pointer constant, not the contents

   void* memory;
   UInt8* bytePtr;
   Int16* wordPtr;
};

}

#endif // L16E_MEMORY_H
