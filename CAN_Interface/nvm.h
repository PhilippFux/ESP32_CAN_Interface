#ifndef NVM_EEPROM_HANDLING_H
#define NVM_EEPROM_HANDLING_H
#include <Arduino.h>


/* ---------- NVM /EEPROM Handling ---------------*/

class Nvm
{
private:
	bool write_needed;
	size_t nvm_size;
	uint8_t *ram_copy;
	uint8_t *temp_config;

public:
	Nvm();
	void init(size_t size, uint8_t *ram_copy);
	void read(void);
	void write(void);
	void require_update();
	void reset_update();
	bool get_update_flag();

};


/* ---------- NVM /EEPROM Handling ---------------*/
#endif
