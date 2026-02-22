//-------------EEPROM Manager header-------------------------------------
// // Copyright (c) 2025 F. Mispelaer
//-----------------------------------------------------------------------
#ifndef _EEPROMManager_h
#define _EEPROMManager_h

#include <Arduino.h>
#include "config.h"

void StoreString(int, String);
String readStoredString(int);
byte readStoredbyte(int);
void StoreByte(int, byte);

extern int addrnames[totalswitches];

#endif