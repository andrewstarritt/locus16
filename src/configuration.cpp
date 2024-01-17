/* configuration.cpp
 *
 * Configuration, part of the Locus 16 Emulator.
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

#include "configuration.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <INIReader.h>
#include "locus16_common.h"
#include "peripheral.h"
#include "tape_punch.h"
#include "tape_reader.h"
#include "terminal.h"

#include "alp_processor.h"
#include "clock.h"
#include "memory.h"
#include "rom.h"
#include "serial.h"


using namespace L16E;

//------------------------------------------------------------------------------
//
Configuration::Configuration () {}

//------------------------------------------------------------------------------
//
Configuration::~Configuration () {}


//------------------------------------------------------------------------------
// static
bool Configuration::readConfiguration (const std::string iniFile,
                                       DataBus* dataBus)
{
   char sectionText [20];
   char hex [20];

   // First check file exists and is readable - this provides better error reports.
   //
   int fd = open(iniFile.c_str(), O_RDONLY);
   if (fd < 0) {
      perror(iniFile.c_str());
      return false;
   }
   close(fd);

   INIReader* c = new INIReader(iniFile);

   int error = c->ParseError();
   if (error != 0) {
      std::cerr << iniFile << ": parse error " << error << "\n";
      return false;
   }

   const int numberDevices = c->GetInteger("System", "NumberDevices", -1);
   if (numberDevices < 1) {
      std::cerr << iniFile << ": no devices specified" << "\n";
      return false;
   }
   const int numberPeripherals = c->GetInteger("System", "NumberPeripherals", 0);

   std::cout << "Number devices:     " << numberDevices << "\n";
   std::cout << "Number peripherals: " << numberPeripherals << "\n";
   std::cout << "\n";

   bool status = true;  // hypothesize all okay.

   Peripheral* peripherals [Peripheral::maximumNumberOfPeripherals + 1]; // Note: slot 0 is not used here.
   for (int j = 0; j < ARRAY_LENGTH (peripherals); j++) peripherals[j] = nullptr;

   // Read the peripherals
   //
   for (int p = 1; p <= numberPeripherals; p++) {
      snprintf(sectionText, sizeof(sectionText), "Peripheral%d", p);

      const std::string kind = c->GetString(sectionText, "Kind", "None");
      std::cout << "peripheral: " << p << "\n";
      std::cout << "  kind:     " << kind << "\n";

      if (kind == "Terminal") {
         peripherals [p] = new L16E::Terminal ();

      } else if (kind == "TapeReader") {
         const std::string defaultName = c->GetString(sectionText, "DefaultName", "");
         std::cout << "  default:  " << defaultName << "\n";

        peripherals [p] = new L16E::TapeReader (defaultName);

      } else if (kind == "TapePunch") {
         const std::string defaultName = c->GetString(sectionText, "DefaultName", "");
         std::cout << "  default:  " << defaultName << "\n";
         peripherals [p] = new L16E::TapePunch (defaultName);

      } else {
         std::cerr  << iniFile << ": unknown peripheral kind\n";
         status = false;
      }
   }
   std::cout << "\n";

   DataBus::Device* device  = nullptr;
   MemoryMapper* mapper = nullptr;

   // Read the devices
   //
   for (int d = 1; d <= numberDevices; d++) {

      snprintf(sectionText, sizeof(sectionText), "Device%d", d);

      const std::string kind = c->GetString(sectionText, "Kind", "undefined");
      std::cout << "device: " << d << "\n";
      std::cout << "  kind:     " << kind << "\n";

      if (kind == "ALP1") {
         const int p = c->GetInteger(sectionText, "Processor", -1);
         const int addr = 0x7F00 - ((p-1)* 0x0100);

         if (p < 1 || p > 2) {
            std::cerr << "  invalid/missing processor number: " << p << "\n";
            status = false;
         }

         std::cout << "  processor no.: " << p << "\n";
         snprintf (hex, sizeof (hex), "=X%04X", addr);
         std::cout << "  address:  " << hex << "\n";

         device = new ALP_Processor (p, ALP_Processor::alp1, dataBus);
         status &= device->getIsRegistered();

      } else if (kind == "MemoryController") {
         const int type = c->GetInteger(sectionText, "Type", 0);
         const int addr = c->GetInteger(sectionText, "Address", -1);

         std::cout << "  type:     " << type << "\n";
         snprintf (hex, sizeof (hex), "=X%04X", addr);
         std::cout << "  address:  " << hex << "\n";

         mapper = new MemoryMapper (dataBus);
         status &= mapper->getIsRegistered();


      } else if (kind == "RAM") {
         const int number = c->GetInteger(sectionText, "Number", -1);
         std::cout << "  number:   " << number << "\n";
         device = new Memory (number, mapper, dataBus);
         status &= device->getIsRegistered();


      } else if (kind == "ROM") {
         const std::string filename = c->GetString(sectionText, "Filename", "");
         std::cout << "  filename: " << filename << "\n";

         device = new ROM (filename, dataBus);
         status &= device->getIsRegistered();


      } else if (kind == "Clock") {
         const int addr = c->GetInteger(sectionText, "Address", -1);
         snprintf (hex, sizeof (hex), "=X%04X", addr);
         std::cout << "  address:  " << hex << "\n";

         device = new Clock (dataBus);
         status &= device->getIsRegistered();


      } else if (kind == "Serial") {
         const std::string type = c->GetString(sectionText, "Type", "");
         const int addr = c->GetInteger(sectionText, "Status", -1);
         const int peripheral = c->GetInteger(sectionText, "Peripheral", -1);

         std::cout << "  type:     " << type << "\n";
         snprintf (hex, sizeof (hex), "=X%04X", addr);
         std::cout << "  status:   " << hex << "\n";
         snprintf (hex, sizeof (hex), "=X%04X", addr+2);
         std::cout << "  data:     " << hex << "\n";

         if (peripheral >= 1 && peripheral <= numberPeripherals) {
            std::cout << "  peripheral: " << peripheral << "\n";

            Serial* serial;
            if (type == "Input") {
               serial = new Serial (Serial::Input, addr, dataBus);
               status &= serial->getIsRegistered();
               serial->connect(peripherals [peripheral]);

            } else if (type == "Output") {
               serial = new Serial (Serial::Output, addr, dataBus);
               status &= serial->getIsRegistered();
               serial->connect(peripherals [peripheral]);

            } else {
               std::cerr << iniFile << ": unknown serial device type\n";
               status = false;
            }

         } else {
            std::cout << iniFile << ": no/invalid peripheral specified" << "\n";
            status = false;
         }

      } else if (kind == "None") {
          // pass

      } else {
         std::cerr << iniFile << ": unknown device kind\n";
         status = false;
      }
   }
   std::cout << "\n";

   return status;
}


// end
