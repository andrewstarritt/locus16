/* peripheral.cpp
 */

#include "peripheral.h"
#include <string.h>
#include <stdarg.h>
#include <iostream>

using namespace L16E;

int Peripheral::count = 0;
Peripheral* Peripheral::crate [maximumNumberOfPeripherals] ={
   NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL,
   NULL, NULL, NULL, NULL, NULL
};

//------------------------------------------------------------------------------
// static
void Peripheral::perrorf (const char* format, ...)
{
   char message [200];
   va_list arguments;
   va_start (arguments, format);
   vsnprintf (message, sizeof (message), format, arguments);
   va_end (arguments);
   perror (message);
}

//------------------------------------------------------------------------------
//
Peripheral::Peripheral(const char* nameIn):
   name (strndup(nameIn, 40))
{
   Peripheral::registerPeripheral (this);
}

//------------------------------------------------------------------------------
//
Peripheral::~Peripheral()
{
}

//------------------------------------------------------------------------------
//
bool Peripheral::initialise()
{
   // Nothing do do here yet
   return true;
}

//------------------------------------------------------------------------------
//
bool Peripheral::readByte(UInt8& value)
{
   std::cerr << "Peripheral::readByte not implmented" << std::endl;
   return false;
}

//------------------------------------------------------------------------------
//
bool Peripheral::writeByte(const UInt8 value)
{
   std::cerr << "Peripheral::writeByte not implmented" << std::endl;
   return false;
}

//------------------------------------------------------------------------------
// static
bool Peripheral::registerPeripheral (Peripheral* peripheral)
{
   if (Peripheral::count >= maximumNumberOfPeripherals) return false;
   Peripheral::crate[count] = peripheral;
   Peripheral::count++;
   return true;
}

//------------------------------------------------------------------------------
// static
bool Peripheral::initialisePeripherals()
{
   bool result = true;   // hypothesize all okay

   for (int p = 0; p < Peripheral::count; p++) {
      Peripheral*  peripheral= Peripheral::crate [p];
      result &= peripheral->initialise();
   }

   return result;
}

//------------------------------------------------------------------------------
//
void Peripheral::listPeripherals()
{
   std::cout << "Available peripherals" << std::endl;
   for (int p = 0; p < Peripheral::count; p++) {
      Peripheral* peripheral = Peripheral::crate [p];

      char buffer [80];

      snprintf(buffer, sizeof (buffer),
               "%2d %-20s", p+1, peripheral->name);

      std::cout << buffer << std::endl;
   }
   std::cout << std::endl;
}


//------------------------------------------------------------------------------
// static
int Peripheral::peripheralCount()
{
   return Peripheral::count;
}

//------------------------------------------------------------------------------
// static
Peripheral* Peripheral::getPeripheral (const int index)
{
   Peripheral* peripheral = nullptr;
   if ((index >= 0) && (index < Peripheral::count)) {
      peripheral = Peripheral::crate [index];
   }
   return peripheral;
}

// end
