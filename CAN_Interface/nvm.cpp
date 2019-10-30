#include "nvm.h"
#include <EEPROM.h>

/* ---------- NVM /EEPROM Handling ---------------*/

Nvm::Nvm() { write_needed = false; }

void Nvm::init(size_t size, uint8_t *given_ram_copy) {
  EEPROM.begin(size + 1);
  nvm_size = size;

  temp_config = new uint8_t[nvm_size];
  ram_copy = given_ram_copy;
}

void Nvm::read(void) {
  uint8_t checksum = 0;
  uint8_t nvChecksum = 0;
  uint8_t temp = 0;

  int i;

  for (i = 0; i < nvm_size; i++) {
    temp = EEPROM.read(i);
       *(ram_copy + i) = temp;
    checksum += temp;
  }

  checksum++;
  nvChecksum = EEPROM.read(i);
  if (checksum == nvChecksum) {
    memcpy(ram_copy, temp_config, nvm_size);
    Serial.println("NVM checksum ok");

  } else {
    write_needed = true;  // force write nvm
    Serial.println("NVM checksum NOK // first startup");
    Serial.println(nvChecksum);
    Serial.println(checksum);
  }
}

void Nvm::write(void) {
  uint8_t checksum = 0;
  unsigned char temp = 0;
  int i = 0;

  for (i = 0; i < nvm_size; i++) {
    temp = *(ram_copy + i);
    EEPROM.write(i, temp);
    checksum += temp;
  }
  checksum++;
  EEPROM.write(i, checksum);
  EEPROM.commit();
}

void Nvm::require_update() { write_needed = true; }

void Nvm::reset_update() { write_needed = false; }

bool Nvm::get_update_flag() { return write_needed; }

/* ---------- NVM /EEPROM Handling ---------------*/
