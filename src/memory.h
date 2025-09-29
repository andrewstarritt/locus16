/* memory.h
 *
 * Creates extended RAM memory addressed from 0x9000 to 0x7000
 * together with a memory controller, part of the Locus 16 Emulator
 * application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
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
   // 0x7B00    - used by primary ALP
   // 0x7B02    - used by secondary ALP
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

   bool initialise ();
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
