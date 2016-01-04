/*
Added few functions on top of I2C driver by:
https://github.com/zarya
in order to provide burst reading using I2C.
*/


#include "i2c_master_ext.h"
#include "i2c_master.h"

/*
  Reads 'len' number of bytes from register 'reg'
*/
bool ICACHE_FLASH_ATTR i2c_read_bytes(uint8 addr, uint8 reg, uint8 *pData, uint16 len)
{
    uint8 ack;
    uint16 i;

    i2c_start();
    // shift address left, I2C write phase
    i2c_writeByte(addr << 1);
    ack = i2c_check_ack();

    if (!ack) {
        os_printf("Error writing on I2C #1: %d \n", ack);
        i2c_stop();
        return false;
    }

    i2c_writeByte(reg);
    ack = i2c_check_ack();
    if (!ack) {
        os_printf("Error writing on I2C #2: %d\n", ack);
        i2c_stop();
        return false;
    }

    i2c_stop();
    /* wait a bit */
    os_delay_us(40000);

    i2c_start();
    // shift address left, increment one -> I2C read after write phase
    i2c_writeByte((addr<<1)|1);
    ack = i2c_check_ack();

    for (i = 0; i < len; i++) {
        pData[i] = i2c_readByte();
        /* send ack on end, nack meanwhile */
        i2c_send_ack((i == (len - 1)) ? 0 : 1);
    }
    /* debug */
    #ifdef DEBUG
      for(i = 0; i < len; i++) {
        os_printf("%s Read response from I2C: %d, %d - %d\n", __FUNCTION__, i, len, pData[i]);
      }
    #endif

    i2c_stop();

    return true;
}

/*
  Write byte 'value' to register 'reg'
*/
bool ICACHE_FLASH_ATTR i2c_write_byte(uint8 addr, uint8 reg, uint8 value)
{
    uint8 ack;
    uint16 i;

    i2c_start();
    i2c_writeByte(addr<<1);
    ack = i2c_check_ack();

    if (!ack) {
        os_printf("Error writing on I2C #3: %d\n", ack);
        i2c_stop();
        return false;
    }

    i2c_writeByte(reg);
    ack = i2c_check_ack();

    if (!ack) {
        os_printf("Error writing on I2C #4: %d\n", ack);
        i2c_stop();
        return false;
    }

    i2c_writeByte(value);
    ack = i2c_check_ack();
    i2c_stop();
    return true;
}
