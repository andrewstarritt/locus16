/* data_bus.cpp
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "data_bus.h"
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <iomanip>

using namespace L16E;

//------------------------------------------------------------------------------
// Create a wrapper around strndup, hide that pesky warning.
//
static const char* duplicate (const char* source)
{
   const int n = strnlen (source, 80);
   char* result = strndup (source, n);
   return result;
}


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
   name(duplicate(nameIn)),
   isActive(isActiveIn)
{
   if (this->isActive) {
      this->activeIdentity = this->dataBus->activeCount++;
   } else {
      this->activeIdentity = -1;
   }
   this->isRegistered = dataBus->registerDevice(this);
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
//
bool DataBus::Device::getIsRegistered() const
{
   return this->isRegistered;
}

//------------------------------------------------------------------------------
// Overlays two bytes onto a 16 bit word.
//
union Data {
   Int16 word;
   UInt8 bytes [2];
};

//------------------------------------------------------------------------------
//
bool DataBus::Device::initialise ()
{
   // Nothing do do here yet
   return true;
}

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
std::string DataBus::Device::addrRange () const
{
   char buffer [20];
   snprintf(buffer, sizeof (buffer), "0x%04X..0x%04X",
            this->addrLow & 0xFFFF, this->addrHigh & 0xFFFF);
   return std::string (buffer);
}


//==============================================================================
// DataBus::Device
//==============================================================================
//
DataBus::ActiveDevice::ActiveDevice(DataBus* const dataBus,
                                    const Int16 addrLow,    // inclusive
                                    const Int16 addrHigh,   // exclusive
                                    const char* name) :
   DataBus::Device (dataBus, addrLow, addrHigh, name, true)
{ }

//------------------------------------------------------------------------------
//
DataBus::ActiveDevice::~ActiveDevice() { }


//------------------------------------------------------------------------------
//
bool DataBus::ActiveDevice::execute()
{
   std::cerr << "Program Error: Active device does not override execute() method."
             << std::endl;
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

   Int16 getWord(const Int16) const { return DataBus::allOnes; }
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
   if (!device) return false;

   if (this->count >= maximumNumberOfDevices) {
      std::cerr << "*** too many devices" << std::endl;
      return false;
   }

   // Check for address overlaps.
   //
   for (int d = 0; d < this->count; d++) {
      Device* other = this->crate [d];
      if ((device->addrHigh > other->addrLow) &&
          (device->addrLow < other->addrHigh)) {
         // overlap
         std::cerr << "*** " << device->name << " " << device->addrRange()
                   << " overlaps "
                   << other->name << " " << other->addrRange() << std::endl;
         return false;
      }
   }

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
bool DataBus::initialiseDevices ()
{
   bool result = true;   // hypothesize all okay

   for (int d = 0; d < this->count; d++) {
      Device* device = this->crate [d];
      result &= device->initialise();
   }

   return result;
}

//------------------------------------------------------------------------------
//
void DataBus::listDevices() const
{
   std::cout << "Available devices" << std::endl;
   std::cout  << " # Type                 Address-range   Active-Id" << std::endl;
   for (int d = 0; d < this->count; d++) {
      Device* device = this->crate [d];
      const bool isActive = device->getIsActive();

      std::cout << std::right << std::setw(2) << d+1 << " "
                << std::left << std::setw(20) << device->name<< " "
                << device->addrRange();
      if (isActive)
         std::cout<< "  *" << device->getActiveIdentity();

      std::cout<< std::endl;
   }
   std::cout << std::endl;
}

//------------------------------------------------------------------------------
//
int DataBus::getActiveDevices (ActiveDevice* deviceList[], const int maxNumber) const
{
   int number = 0;
   if (!deviceList) return 0;  // sanity check

   for (int d = 0; (d < this->count) && (number < maxNumber); d++) {
      ActiveDevice* device = dynamic_cast <ActiveDevice*> (this->crate [d]);
      if (device) {
         deviceList [number] = device;
         number++;
      }
   }

   return number;
}

//------------------------------------------------------------------------------
//
int DataBus::deviceCount() const
{
   return this->count;
}

//------------------------------------------------------------------------------
//
DataBus::Device* DataBus::getDevice (const int index) const
{
   Device* device = nullptr;
   if ((index >= 0) && (index < this->count)) {
      device = this->crate [index];
   }
   return device;
}

// end
