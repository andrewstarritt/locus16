/* locus16_main.cpp
 *
 * Locus 16 Emulator
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
static bool startsWith(const char* text, const char* with,
                       const bool useCase = false)
{
   if (!text || !with) return false;
   int n = strnlen(with, 256);
   int c = useCase ? strncmp(text, with, n) : strncasecmp(text, with, n);

   return c == 0;
}

//------------------------------------------------------------------------------
//
int run (const std::string romFile,
         const std::string programFile,
         const std::string outputFile,
         const int sleepModulo)
{
   L16E::DataBus* const dataBus = new L16E::DataBus();

   // Create Devices
   // For actived devices (e.g. ALPs), the order defines the active identity.
   //
   L16E::MemoryMapper* const mapper = new L16E::MemoryMapper (dataBus);
   L16E::Memory* const memory = new L16E::Memory (74, mapper, dataBus);
   L16E::ROM* const rom = new L16E::ROM (dataBus);
   L16E::Clock* const clock = new L16E::Clock (dataBus);
   L16E::ALP_Processor* const processor1 = new L16E::ALP_Processor (1, L16E::ALP_Processor::alp1, dataBus);
// L16E::ALP_Processor* const processor2 = new L16E::ALP_Processor (2, L16E::ALP_Processor::alp1, dataBus);
   L16E::Serial* const vduInputSerial   = new L16E::Serial (L16E::Serial::Input,  0x7B10, dataBus);
   L16E::Serial* const vduOutputSerial  = new L16E::Serial (L16E::Serial::Output, 0x7B14, dataBus);
   L16E::Serial* const tapeReaderSerial = new L16E::Serial (L16E::Serial::Input,  0x7B18, dataBus);
   L16E::Serial* const tapePunchSerial  = new L16E::Serial (L16E::Serial::Input,  0x7B1C, dataBus);

   // Non-devices that are also "on the" data bus.
   //
   L16E::Diagnostics* const diagnostics = new L16E::Diagnostics (dataBus);

   // Create Peripherals
   //
   L16E::Terminal* const terminal = new L16E::Terminal ();
   L16E::TapeReader* const tapeReader = new L16E::TapeReader (programFile.c_str());
   L16E::TapePunch* const tapePunch = new L16E::TapePunch (outputFile.c_str());

   L16E::DataBus::Device* activeDeviceList [L16E::DataBus::maximumNumberOfDevices];

   // List all available devices.
   //
   dataBus->listDevices();

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

   // Initialise devices and peripherals.
   //
   rom->initialise(romFile.c_str());
   memory->initialise(0);
   terminal->initialise();
   tapeReader->initialise();
   tapePunch->initialise();

   // Connect devices to the peripherals
   //
   vduInputSerial->connect(terminal);
   vduOutputSerial->connect(terminal);
   tapeReaderSerial->connect(tapeReader);
   tapePunchSerial->connect(tapePunch);

   signal (SIGINT,  signalCatcher);

   std::cout << std::endl;

   processor1->dumpRegisters();

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
         // We did think about a separate thread for each active device, however
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

            // Select the active device.
            //
            L16E::DataBus::Device* device = activeDeviceList [activeDevice];

            // Let memory mapper controller know who is (or will be)
            // trying to access memory.
            //
            int id = device->getActiveIdentity();
            mapper->setActiveIdentity(id);

            // Check for break points.
            //
            if ((ic > 0) && (device == processor1)) {
               Int16 next = processor1->getPreg();
               if (diagnostics->isBreakPoint(next)) {
                  // At a break point
                  std::cout << "break point " << device->getName() << std::endl;
                  break;
               }
            }

            if (clock->testAndClearInterruptPending()) {
               processor1->requestInterrupt();
            }

            bool status = device->execute();

            // This slows the emulator down to approximatley real-time
            // At least on my setup at home.
            //
            if ((ic % sleepModulo) == 0) usleep (1);

            // Let clock know we have executed one instruction.
            // This add 2.25 uSec to the amount of time that has passed.
            // (based on on active/ALP process in the crate).
            //
            clock->executeCycle ();

            // do the round-robin update.
            activeDevice = (activeDevice + 1) % activeCount;

            if (!status) {
               // The device reports the error.
               diagnostics->accessAddress(processor1->getPreg() - 2);
               break;
            }
         }
         //
         /// -------------------------------------------------------------------

         processor1->dumpRegisters();
         diagnostics->accessAddress(processor1->getPreg());

      } else if (startsWith(start, "AA")) {
         // Access address
         int n;
         int addr = 0;
         int words = 1;

         n = sscanf(start + 2, "%x %d", &addr, &words);

         if (n >= 1) {
            diagnostics->accessAddress(addr, addr + 2*words);
         } else {
            std::cout << "Invalid:" << start << std::endl;
         }

      } else if (startsWith(start, "DM")) {
         // Dump memory
         int n;
         int addr = 0;
         int words = 1;

         n = sscanf(start + 2, "%x %d", &addr, &words);

         if (n >= 1) {
            diagnostics->wideDump(addr, addr + 2*words);
         } else {
            std::cout << "Invalid:" << start << std::endl;
         }

      } else if (startsWith(start, "DR")) {
         // Dump registers
         processor1->dumpRegisters();

      } else if (startsWith(start, "SB")) {
         // Set break
         int n;
         int addr = 0;

         n = sscanf(start + 2, "%x", &addr);
         if (n == 1) {
            // Set break point.
            diagnostics->setBreak (addr);
         } else {
            std::cout << "Invalid:" << start << std::endl;
         }

      } else if (startsWith(start, "CB")) {
         // Clear break
         int n;
         int addr = 0;

         n = sscanf(start + 2, "%x", &addr);
         if (n == 1) {
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
               "DR                   dump ALP registers for current level\n"
               "SB hexaddr           set break point\n"
               "CB hexaddr           clear break point\n"
               "LB                   list break points\n"
               "HE                   help\n"
               "// <any text>        comment - ignored.\n";

         std::cout << hlp;

      } else {
         std::cout << "Invalid command: " << start << std::endl;
      }

   loopContinue:
      if (lastLine) free (lastLine);
      lastLine = thisLine;
      thisLine = nullptr;
   }

   printf ("complete\n");
   return 0;
}

// end
