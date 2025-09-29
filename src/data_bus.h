/* data_bus.h
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#ifndef L16E_DATA_BUS_H
#define L16E_DATA_BUS_H

#include "locus16_common.h"
#include <string>

namespace L16E {

/// Essentally the system.
///
class DataBus {
public:
   enum Constants {
      // Addresses, a, are  first <= a <= last
      //
      addressFirst = (-32768),    // 0x8000
      addressLast  = (+32767),    // 0x7FFF
      allOnes      = (-1),        // 0xFFFF

      // Some usefull addresses
      //
      X8000    = -32768,
      X9000    = -28672,
      XA000    = -24576,
      XB000    = -20480,
      XC000    = -16384,
      XD000    = -12288,
      XE000    = -8192,
      XF000    = -4096,
      X0000    = 0,
      X1000    = 4096,
      X2000    = 8192,
      X3000    = 12288,
      X4000    = 16384,
      X5000    = 20480,
      X6000    = 24576,
      X7000    = 28672,

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
      bool getIsRegistered() const;

      // Concrete devices must specify these functions.
      //
      virtual Int16 getWord(const Int16 addr) const = 0;
      virtual void setWord(const Int16 addr, const Int16 value) = 0;

      // Concrete devices may override these functions.
      //
      virtual bool initialise ();
      virtual UInt8 getByte(const Int16 addr) const;
      virtual void setByte(const Int16 addr, const UInt8 value);

      std::string addrRange () const;

   protected:
      DataBus* const dataBus;  // the pointer/reference is constant, not the object
      const Int16 addrLow;
      const Int16 addrHigh;
      const char* const name;
      const bool isActive;
      int activeIdentity;
      bool isRegistered;

      friend class DataBus;
   };


   // Any active device (such as ALP, DMA, PPI) should inherit from this class.
   //
   class ActiveDevice : public Device {
   public:
      explicit ActiveDevice(DataBus* const dataBus,
                            const Int16 addrLowIn,    // inclusive
                            const Int16 addrHighIn,   // exclusive
                            const char* name);
      virtual ~ActiveDevice();

      // Must be overriden by active devices such the the ALP processor.
      //
      virtual bool execute();
   };

   explicit DataBus();
   ~DataBus();

   UInt8 getByte(const Int16 addr) const;
   void setByte(const Int16 addr, const UInt8 value);

   Int16 getWord(const Int16 addr) const;
   void setWord(const Int16 addr, const Int16 value);

   bool initialiseDevices ();
   void listDevices() const;  // prints to stdout

   int getActiveDevices (ActiveDevice* deviceList[], const int maxNumber) const;

   int deviceCount() const;
   Device* getDevice (const int index) const;

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
