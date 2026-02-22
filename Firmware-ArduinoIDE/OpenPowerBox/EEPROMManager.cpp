#include "EEPROMManager.h"
#include <Arduino.h>
#include <EEPROM.h>


int addrnames[totalswitches];

//===========================================================================
//============================ EEPROM FUNCTIONS =============================
//===========================================================================

void StoreString(int adress, String str)
{
  int len = str.length() + 1;
  char buf[len];
  str.toCharArray(buf, len);
  for (int i = 0; i < len; i++)
    EEPROM.put((adress + i), buf[i]);
}

void StoreByte(int adress, byte val)
{
  EEPROM.put((adress), val);
}

String readStoredString(int adress)
{
  char buf[20];
  EEPROM.get(adress, buf);
  String str = String(buf);
  return str;
}

byte readStoredbyte(int adress)
{
  byte buf;
  EEPROM.get(adress, buf);
  return buf;
}