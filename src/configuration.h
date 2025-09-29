/* configuration.h
 *
 * Configuration, part of the Locus 16 Emulator.
 *
 * SPDX-FileCopyrightText: 2022-2025  Andrew C. Starritt
 * SPDX-License-Identifier: LGPL-3.0-only
 *
 * Contact details:
 * andrew.starritt@gmail.com
 */

#ifndef L16E_CONFIGURATION_H
#define L16E_CONFIGURATION_H

#include <string>
#include "data_bus.h"

namespace L16E {

class Configuration {
public:
   // Reads configuration data and creates the specified peripherals and devices.
   //
   static bool readConfiguration (const std::string iniFile,
                                  DataBus* dataBus);
private:
   explicit Configuration ();
   ~Configuration ();
};

}

#endif // L16E_CONFIGURATION_H
