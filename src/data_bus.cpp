/* data_bus.cpp
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

#include "data_bus.h"
#include <stdio.h>
#include <string.h>
#include <iostream>

using namespace L16E;

//==============================================================================
// DataBus::Device
//==============================================================================
//
DataBus::Device::Device(DataBus* const dataBusIn,
                        const Int16 addrLowIn,
                        const Int16 addrHighIn,
                        const char* nameIn,
                        const bool isActiveIn) :
   dataBus(dataBusIn),
   addrLow(addrLowIn),
   addrHigh(addrHighIn),
   name (strndup(nameIn, 40)),
   isActive(isActiveIn)
{
   if (this->isActive) {
      this->activeIdentity = this->dataBus->activeCount++;
   } else {
      this->activeIdentity = -1;
   }
   dataBus->registerDevice(this);
}

DataBus::Device::~Device() {}

//------------------------------------------------------------------------------
//
DataBus* const DataBus::Device::owner() const
{
   return this->dataBus;
}

//------------------------------------------------------------------------------
//
Int16 DataBus::Device::getAddrLow () const
{
   return this->addrLow;
}

//------------------------------------------------------------------------------
//
Int16 DataBus::Device::getAddrHigh () const
{
   return this->addrHigh;
}

//------------------------------------------------------------------------------
//
const char* DataBus::Device::getName() const
{
   return this->name;
}

//------------------------------------------------------------------------------
//
bool DataBus::Device::getIsActive () const
{
   return this->isActive;
}

//------------------------------------------------------------------------------
//
int  DataBus::Device::getActiveIdentity() const
{
   return this->activeIdentity;
}

//------------------------------------------------------------------------------
// Overlays two bytes onto a 16 bit word.
//
union Data {
   Int16 word;
   UInt8 bytes [2];
};

//------------------------------------------------------------------------------
// Default device byte addressing based on word addressing
//
UInt8 DataBus::Device::getByte(const Int16 addr) const
{
   Data data;
   data.word = this->getWord (addr & 0xFFFE);

   // For even address we want most significant byte, and vice versa
   // This is because locus 16 words are stored big endian in memory
   //
   return data.bytes [(addr & 1) ^ 1];
}

//------------------------------------------------------------------------------
//
void DataBus::Device::setByte(const Int16 addr, const UInt8 value)
{
   Data data;

   data.word = this->getWord (addr & 0xFFFE);
   data.bytes [(addr & 1) ^ 1] = value;
   this->setWord (addr & 0xFFFE, data.word);
}

//------------------------------------------------------------------------------
//
bool DataBus::Device::execute()
{
   if (this->isActive) {
      std::cerr << "Program Error: Active device does not override fetchExecute" << std::endl;
   } else {
      std::cerr << "Program Error: fetchExecute invoked for non active device" << std::endl;
   }
   return false;
}


//==============================================================================
// NullDevice
//==============================================================================
//
class NullDevice : public DataBus::Device {
public:
   explicit NullDevice(DataBus* const dataBus) :
      DataBus::Device  (dataBus, 0, 0, "Null", false) { }
   virtual ~NullDevice() { }

   UInt8 getByte(const Int16) const { return 0xFF; }
   void setByte(const Int16, const UInt8) { }

   Int16 getWord(const Int16) const { return 0xFFFF; }
   void setWord(const Int16, const Int16) { }
};


//==============================================================================
// DataBus
//==============================================================================
//
DataBus::DataBus()
{
   this->count = 0;
   this->activeCount = 0;
   this->nullDevice = new NullDevice (this);

   // Reset registration counter to exclude the null device.
   //
   this->count = 0;
   this->activeCount = 0;

   for (int d = 0; d < maximumNumberOfDevices; d++) {
      this->crate [d] = nullptr;
   }
}

//------------------------------------------------------------------------------
//
DataBus::~DataBus() {}

//------------------------------------------------------------------------------
//
bool DataBus::registerDevice(Device* device)
{
   if (this->count >= maximumNumberOfDevices) return false;
   this->crate[this->count] = device;
   this->count++;
   return true;
}


//------------------------------------------------------------------------------
// Esssential we dispatch via databus address
//
DataBus::Device* DataBus::findDevice (const Int16 addr) const
{
   Device* result = this->nullDevice;

   for (int d = 0; d < this->count; d++) {
      Device* device = this->crate [d];
      if ((addr >= device->addrLow) && (addr < device->addrHigh)) {
         result = device;
         break;
      }
   }

   return result;
}

//------------------------------------------------------------------------------
//
UInt8 DataBus::getByte (const Int16 addr) const
{
   Device* device = DataBus::findDevice (addr);
   return device->getByte(addr);
}

//------------------------------------------------------------------------------
//
void DataBus::setByte(const Int16 addr, const UInt8 value)
{
   Device* device = DataBus::findDevice (addr);
   device->setByte(addr, value);
}

//------------------------------------------------------------------------------
//
Int16 DataBus::getWord(const Int16 addr) const
{
   Device* device = DataBus::findDevice (addr);
   return device->getWord(addr);
}

//------------------------------------------------------------------------------
//
void DataBus::setWord(const Int16 addr, const Int16 value)
{
   Device* device = DataBus::findDevice (addr);
   device->setWord(addr, value);
}

//------------------------------------------------------------------------------
//
void DataBus::listDevices() const
{
   std::cout << "Available devices" << std::endl;
   for (int d = 0; d < this->count; d++) {
      Device* device = this->crate [d];

      char buffer [80];

      snprintf(buffer, sizeof (buffer),
               "%2d %-20s 0x%04X  0x%04X %s", d+1, device->name,
               device->addrLow & 0xFFFF,
               device->addrHigh & 0xFFFF,
               device->isActive ? "*" : "");

      std::cout << buffer << std::endl;
   }
   std::cout << std::endl;
}

//------------------------------------------------------------------------------
//
int DataBus::getActiveDevices (Device* deviceList[], const int maxNumber) const
{
   int number = 0;
   if (!deviceList) return 0;  // sanity check

   for (int d = 0; (d < this->count) && (number < maxNumber); d++) {
      Device* device = this->crate [d];
      if (device->getIsActive()) {
         deviceList [number] = device;
         number++;
      }
   }

   return number;
}

// end
