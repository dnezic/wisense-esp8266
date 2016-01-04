#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"

#define I2C_SLEEP_TIME 10

#define I2C_SDA_MUX PERIPHS_IO_MUX_GPIO2_U
#define I2C_SDA_FUNC FUNC_GPIO2
#define I2C_SDA_PIN 2

#define I2C_SCK_MUX PERIPHS_IO_MUX_MTMS_U
#define I2C_SCK_FUNC FUNC_GPIO14
#define I2C_SCK_PIN 14

//SCK on GPIO0 (untested)
//#define I2C_SCK_MUX PERIPHS_IO_MUX_GPIO0_U
//#define I2C_SCK_PIN 0
//#define I2C_SCK_FUNC FUNC_GPIO0

#define i2c_read() GPIO_INPUT_GET(GPIO_ID_PIN(I2C_SDA_PIN));

void i2c_init(void);
void i2c_start(void);
void i2c_stop(void);
void i2c_send_ack(uint8 state);
uint8 i2c_check_ack(void);
uint8 i2c_readByte(void);
void i2c_writeByte(uint8 data);
