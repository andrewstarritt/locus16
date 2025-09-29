/* locus16_main.cpp
 *
 * Locus 16 Emulator
 *
 * This file is part of the Locus 16 Emulator application.
 *
 * SPDX-FileCopyrightText: 2021-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#include "execute.h"

#include <iostream>
#include <string>
#include <stdio.h>
#include <ctype.h>
#include <cinttypes>
#include <readline/readline.h>
#include <readline/history.h>
#include <signal.h>
#include <unistd.h>

#include "locus16_common.h"
#include "alp_processor.h"
#include "clock.h"
#include "configuration.h"
#include "data_bus.h"
#include "diagnostics.h"
#include "memory.h"
#include "rom.h"
#include "serial.h"
#include "tape_punch.h"
#include "tape_reader.h"
#include "terminal.h"

#define TRACE std::cerr << __LINE__ << " " << __FUNCTION__ << "\n";

//------------------------------------------------------------------------------
//
static bool sigIntReceived = false;
static void signalCatcher (int sig)
{
   switch (sig) {

      case SIGINT:
         if (!sigIntReceived) {
            std::cout << "\nSIGINT received\n";
         }
         sigIntReceived = true;
         break;

      default:
         break;
   }
}

//------------------------------------------------------------------------------
//
static bool startsWith(const char* text, const char* with)
{
   if (!text || !with) return false;
   int n = strnlen(with, 256);
   int c = strncasecmp(text, with, n);
   return c == 0;
}

//------------------------------------------------------------------------------
//
template<class PeripheralType>
static PeripheralType* findPeripheral ()
{
   const int n = L16E::Peripheral::peripheralCount();
   for (int p = 0; p < n; p++) {
      PeripheralType* peripheral = dynamic_cast <PeripheralType*> (L16E::Peripheral::getPeripheral(p));
      if (peripheral) {
         // Found it (or the first at least)
         return peripheral;
      }
   }
   return nullptr;
}

//------------------------------------------------------------------------------
//
template<class DeviceType>
static DeviceType* findDevice (L16E::DataBus* const dataBus, int ordinal = 1)
{
   const int n = dataBus->deviceCount();
   for (int d = 0; d < n; d++) {
      DeviceType* device = dynamic_cast <DeviceType*> (dataBus->getDevice(d));
      if (device) {
         // Found it the required class
         if (ordinal == 1) {
            // Found it the required instance
            return device;
         }
         ordinal--;
      }
   }
   return nullptr;
}

//------------------------------------------------------------------------------
//
int run (const std::string iniFile,
         const std::string programFile,
         const std::string outputFile,
         const int sleepModulo)
{
   bool status;
   L16E::DataBus* const dataBus = new L16E::DataBus();
   L16E::Diagnostics* const diagnostics = new L16E::Diagnostics (dataBus);

   status = L16E::Configuration::readConfiguration(iniFile, dataBus);
   if (!status) return 4;

   // List all available peripherals and devices.
   // Bah!! This is a bit asymetric.
   //
   L16E::Peripheral::listPeripherals();
   dataBus->listDevices();

   L16E::DataBus::ActiveDevice* activeDeviceList [L16E::DataBus::maximumNumberOfDevices];

   // Get a list of all the active devices, e.g. ALP processors, DMA devices etc.
   //
   const int activeCount = dataBus->getActiveDevices (activeDeviceList,
                                                      ARRAY_LENGTH(activeDeviceList));

   printf ("Number of active devices: %d\n", activeCount);
   if (activeCount <= 0) {
      printf ("Incomplete crate - no active devices\n");
      return 2;
   }
   std::cout << std::endl;

   // Load program file "tape" into the tape reader.
   //
   L16E::TapeReader* reader = findPeripheral<L16E::TapeReader>();
   if (reader) {
      reader->setFilename (programFile);
   }

   L16E::TapePunch* punch  = findPeripheral<L16E::TapePunch>();
   if (punch) {
      punch->setFilename (outputFile);
   }

   // Initialise peripherals and devices.
   //
   status = L16E::Peripheral::initialisePeripherals();
   if (!status) return 4;
   status = dataBus->initialiseDevices();
   if (!status) return 4;

   L16E::ALP_Processor* processor1 = findDevice <L16E::ALP_Processor> (dataBus, 1);
   L16E::ALP_Processor* processor2 = findDevice <L16E::ALP_Processor> (dataBus, 2);
   L16E::MemoryMapper* mapper = findDevice <L16E::MemoryMapper> (dataBus);
   L16E::Clock* clock = findDevice <L16E::Clock> (dataBus);;

   // Catch interrupts to allow the emulator to escape program execution and
   // enter into diagnostic mode.
   //
   signal (SIGINT, signalCatcher);

   std::cout << std::endl;

   if (processor1) processor1->dumpRegisters();
   if (processor2) processor2->dumpRegisters();

   char* lastLine = nullptr;
   char* thisLine = nullptr;

   int activeDevice = 0;
   while (true) {
      thisLine = readline ("> ");
      if (!thisLine) {
         std::cerr << "input terminated" << std::endl;
         break;
      }

      // Parse the command response.
      // First convert to a old style bunch of characters.
      // And remember to free buffer
      //
      int first = 0;
      int last = strnlen(thisLine, 256);

      while ((first < last) && isspace(int(thisLine[first]))) first++;
      const char* start = &thisLine[first];

      while ((first < last) && isspace(int(thisLine[last-1]))) last--;
      thisLine[last] = '\0';

      // If not an empty line, then add to the history.
      //
      if (last > first) {
         // Check if repeat of previous line??
         if (!lastLine ||
             strncasecmp(lastLine, thisLine, 256) != 0)
         {
            add_history (start);
         }
      }

      // Recall last is one beyond end of input - kind of python slice style.
      //
      if ((first == last) || startsWith (start, "//")) {
         // Empty line or comment
         // Do nothing
         //

      } else if (startsWith (start, "EX")) {
         // Exit
         std::cout << "exiting..." << std::endl;
         break;  // all done

      } else if (startsWith(start, "CU") || startsWith(start, "SS")) {
         // Continue/run/step
         int64_t number = 1;

         if (startsWith(start, "CU")) {
            if (first + 2 == last) {
               number = 0x7FFFffffFFFFffff;  // "infinity", well actually ~9.2e+18
            } else {
               int n;
               int64_t temp = 0;
               n = sscanf(start + 2, "%ld", &temp);

               if (n == 1) {
                  number = temp;
               } else {
                  std::cout << "Invalid number: " << start << std::endl;

                  /// GOTO end of loop - continue just won't cut the mustard!!!
                  ///
                  goto loopContinue;
               }
            }
         }

         /// -------------------------------------------------------------------
         // Round robin all active devices.
         // I did think about a separate thread for each active device, however
         // the use of mutex prob. negates the benefit of multiple threads.
         //
         sigIntReceived = false;
         for (int64_t ic = 0; ic < number; ic++) {
            // First check for any user command-line interrupt.
            //
            if (sigIntReceived) {
               sigIntReceived = false;
               break;
            }

            // Do the round-robin update and select the active device.
            //
            activeDevice = (activeDevice + 1) % activeCount;
            L16E::DataBus::ActiveDevice* device = activeDeviceList [activeDevice];
            L16E::ALP_Processor* processor = dynamic_cast <L16E::ALP_Processor*> (device);

            // Let memory mapper controller know who is (or will be)
            // trying to access memory.
            //
            int id = device->getActiveIdentity();
            if (mapper) mapper->setActiveIdentity(id);

            // Check for break points.
            //
            if ((ic > 0) && processor) {
               Int16 next = processor->getPreg();
               if (diagnostics->isBreakPoint(next)) {
                  // At a break point
                  std::cout << "break point " << device->getName() << std::endl;
                  break;
               }
            }

            // Only primary ALP gets interrupted by the clock.
            //
            if (clock && clock->testAndClearInterruptPending()) {
               if (processor1) processor1->requestInterrupt();
            }

            bool status = device->execute();

            // This slows the emulator down to approximatley real-time
            // At least on my setup at home.
            //
            if ((ic % sleepModulo) == 0) usleep (1);

            // Let clock know we have executed one instruction.
            // This adds 2.25 or 1.68 uSec to the amount of time that has passed.
            // (based on the number of active/ALP process in the crate).
            //
            if (clock) clock->executeCycle ();

            if (!status) {
               // The device reports the error.
               if (processor) diagnostics->accessAddress(processor->getPreg() - 2);
               break;
            }
         }
         //
         /// -------------------------------------------------------------------

         if (processor1) {
            processor1->dumpRegisters();
            diagnostics->accessAddress(processor1->getPreg());
         }

         if (processor2) {
            processor2->dumpRegisters();
            diagnostics->accessAddress(processor2->getPreg());
         }

      } else if (startsWith(start, "AA")) {
         // Access address
         int n;
         unsigned uaddr = 0;
         int words = 1;

         n = sscanf(start + 2, "%x %d", &uaddr, &words);

         if (n >= 1) {
            const Int16 addr = Int16 (uaddr);
            diagnostics->accessAddress(addr, addr + 2*words);
         } else {
            std::cout << "Invalid: " << start << std::endl;
         }

      } else if (startsWith(start, "DM")) {
         // Dump memory
         int n;
         unsigned uaddr = 0;
         int words = 1;

         n = sscanf(start + 2, "%x %d", &uaddr, &words);

         if (n >= 1) {
            const Int16 addr = Int16 (uaddr);
            diagnostics->wideDump(addr, addr + 2*words);
         } else {
            std::cout << "Invalid: " << start << std::endl;
         }

      } else if (startsWith(start, "SC")) {
         // Set "core" memory
         int n;
         unsigned base = 0;
         unsigned v [20];

         n = sscanf(start + 2, "%x"
                    " %x %x %x %x %x %x %x %x"
                    " %x %x %x %x %x %x %x %x"
                    " %x %x %x %x",
                    &base,
                    &v[0],  &v[1],  &v[2],  &v[3],  &v[4],  &v[5],  &v[6],  &v[7],
                    &v[8],  &v[9],  &v[10], &v[11], &v[12], &v[13], &v[14], &v[15],
                    &v[16], &v[17], &v[18], &v[19]);

         if (n >= 1) {
            for (int j = 0; j < n - 1; j++) {
               const Int16 addr = Int16(base) + 2*j;
               dataBus->setWord(addr, Int16(v[j]));
               diagnostics->accessAddress(addr);
            }
         } else {
            std::cout << "Invalid: " << start << std::endl;
         }

      } else if (startsWith(start, "DR")) {
         // Dump registers
         //
         int n;
         int level;

         n = sscanf(start + 2, "%d", &level);
         if (n >= 1) {
            if (level < 0 || level >= 4) {
               std::cout << "Invalid: level " << level << std::endl;
            } else {
               if (processor1) processor1->dumpRegisters(level);
               if (processor2) processor2->dumpRegisters(level);
            }
         } else {
            if (processor1) processor1->dumpRegisters();
            if (processor2) processor2->dumpRegisters();
         }

      } else if (startsWith(start, "SB")) {
         // Set break
         int n;
         unsigned uaddr = 0;

         n = sscanf(start + 2, "%x", &uaddr);
         if (n == 1) {
            // Set break point.
            const Int16 addr = Int16 (uaddr);
            diagnostics->setBreak (addr);
         } else {
            std::cout << "Invalid:" << start << std::endl;
         }

      } else if (startsWith(start, "CB")) {
         // Clear break
         int n;
         unsigned uaddr = 0;

         n = sscanf(start + 2, "%x", &uaddr);
         if (n == 1) {
            const Int16 addr = Int16 (uaddr);
            diagnostics->clearBreak (addr);
         } else {
            std::cout << "Invalid:" << start << std::endl;
         }

      } else if (startsWith (start, "LB")) {
         // List breaks
         diagnostics->listBreaks();

      } else if (startsWith (start, "HE")) {
         // Help
         const char* hlp;
         hlp = "EX                   exit\n"
               "CU [number]          continue, optional number of instructions\n"
               "SS                   step 1 instruction, same as CU 1\n"
               "AA hexaddr [number]  access address, optional number of words\n"
               "DM hexaddr [number]  dump memory, optional number of words\n"
               "SC hexaddr hexvalues set upto 16 values from the specified start address\n"
               "DR [level]           dump ALP registers for current or specified level\n"
               "SB hexaddr           set break point\n"
               "CB hexaddr           clear break point\n"
               "LB                   list break points\n"
               "HE                   help\n"
               "// <any text>        comment - ignored.\n";

         std::cout << hlp;

      } else {
         std::cout << "Invalid command: " << start << std::endl;
      }

   loopContinue:   // yes - it's a goto - get over it !!
      if (lastLine) free (lastLine);
      lastLine = thisLine;
      thisLine = nullptr;
   }

   printf ("complete\n");
   return 0;
}

// end
