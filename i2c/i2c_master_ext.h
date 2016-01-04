#include "debug.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

bool ICACHE_FLASH_ATTR i2c_read_bytes(uint8 addr, uint8 reg, uint8 *pData, uint16 len);
bool ICACHE_FLASH_ATTR i2c_write_byte(uint8 addr, uint8 reg, uint8 value);
