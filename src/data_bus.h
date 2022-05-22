/* data_bus.h
 *
 * This file is part of the Locus 16 Emulator application.
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

#ifndef L16E_DATA_BUS_H
#define L16E_DATA_BUS_H

#include "locus16_common.h"

namespace L16E {

class DataBus {
public:
   enum Constants {
      // Addresses, a, are  first <= a <= last
      //
      addressFirst = (-32768),    // 0x8000
      addressLast =  (+32767),    // 0x7FFF
      maximumNumberOfDevices = 20
   };

   //---------------------------------------------------------------------------
   // Device, typically a card, is something that plugs into the bus.
   // Devices include memory, ROM and ALP processors.
   // Processors are active, RAM and ROM pas passive.
   //
   class Device {
   public:
      explicit Device(DataBus* const dataBus,
                      const Int16 addrLowIn,    // inclusive
                      const Int16 addrHighIn,   // exclusive
                      const char* name,         // diagonstic
                      const bool isActive);
      virtual ~Device();

      DataBus* const owner() const;
      Int16 getAddrLow() const;
      Int16 getAddrHigh() const;
      const char* getName() const;
      bool getIsActive() const;
      int getActiveIdentity() const;  // return -1 for non-active devices

      // Concrete devices must specify these functions.
      //
      virtual Int16 getWord(const Int16 addr) const = 0;
      virtual void setWord(const Int16 addr, const Int16 value) = 0;

      // Concrete devices may override these functions.
      //
      virtual UInt8 getByte(const Int16 addr) const;
      virtual void setByte(const Int16 addr, const UInt8 value);

      // Must be overriden by active devices such the the ALP processor.
      //
      virtual bool execute();

   protected:
      DataBus* const dataBus;  // the pointer/reference is constant, not the object
      const Int16 addrLow;
      const Int16 addrHigh;
      const char* const name;
      const bool isActive;
      int activeIdentity;

      friend class DataBus;
   };

   explicit DataBus();
   ~DataBus();

   UInt8 getByte(const Int16 addr) const;
   void setByte(const Int16 addr, const UInt8 value);

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16 value);

   void listDevices() const;  // prints to stdout
   int getActiveDevices (Device* deviceList[], const int maxNumber) const;

private:
   bool registerDevice(Device* device);

   // Finds the device, if any, associated with the address.
   //
   Device* findDevice (const Int16 addr) const;

   int count;
   int activeCount;
   Device* crate [maximumNumberOfDevices];
   Device* nullDevice;
};

}

#endif // L16E_DATA_BUS_H
