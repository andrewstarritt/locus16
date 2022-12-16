/* peripheral.h
 */

#ifndef L16E_PERIPHERAL_H
#define L16E_PERIPHERAL_H

#include "locus16_common.h"

namespace L16E {

class Peripheral
{
public:
   enum Constants {
      maximumNumberOfPeripherals = 20
   };

   explicit Peripheral(const char* name);
   ~Peripheral();

   virtual bool initialise();
   virtual bool readByte(UInt8& value);
   virtual bool writeByte(const UInt8 value);

   static bool initialisePeripherals();
   static void listPeripherals();  // prints to stdout

   static int peripheralCount();
   static Peripheral* getPeripheral (const int index);

protected:
   const char* const name;

   // Formatted perror function
   static void perrorf (const char* format, ...);

private:
   static bool registerPeripheral (Peripheral* peripheral);
   static int count;
   static Peripheral* crate [maximumNumberOfPeripherals];
};

}

#endif // L16E_PERIPHERAL_H
