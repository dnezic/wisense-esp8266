/*
   Author: Drazen Nezic
   BME280 driver, adapted for ESP8266, rewritten in C from here:
   https://github.com/kbrownlees/bme280
 */

#include "i2c_master_ext.h"
#include "bme280.h"
#include "c_types.h"
#include "osapi.h"

LOCAL uint8 raw_data[32];
LOCAL uint8 raw[8];
LOCAL long _calibration_t[3];
LOCAL long _calibration_p[9];
LOCAL long _calibration_h[6];
LOCAL float t_fine;

bool ICACHE_FLASH_ATTR user_bme280_read_bytes(uint8 addr, uint8 *data, uint8 length)
{
        return i2c_read_bytes(BME280_addr, addr, data, length);
}

bool ICACHE_FLASH_ATTR user_bme280_write_byte(uint8 addr, uint8 byte)
{
        return i2c_write_byte(BME280_addr, addr, byte);
}

/* reads calibration data from BME280 */
bool ICACHE_FLASH_ATTR read_calibration_data() {

        uint8 i;
        user_bme280_read_bytes(0x88, &raw_data[0], 24);
        user_bme280_read_bytes(0xA1, &raw_data[24], 1);
        user_bme280_read_bytes(0xE1, &raw_data[25], 7);

        i = 0;
        _calibration_t[i++] = (raw_data[1] << 8) | raw_data[0];
        _calibration_t[i++] = (raw_data[3] << 8) | raw_data[2];
        _calibration_t[i++] = (raw_data[5] << 8) | raw_data[4];

    #ifdef DEBUG
        for(i = 0; i < 3; i++) {
                os_printf("%s Calibration data for temperature: %d %d \n", __FUNCTION__, i, _calibration_t[i]);
        }
    #endif

        i = 0;
        _calibration_p[i++] = (raw_data[7] << 8) | raw_data[6];
        _calibration_p[i++] = (raw_data[9] << 8) | raw_data[8];
        _calibration_p[i++] = (raw_data[11] << 8) | raw_data[10];
        _calibration_p[i++] = (raw_data[13] << 8) | raw_data[12];
        _calibration_p[i++] = (raw_data[15] << 8) | raw_data[14];
        _calibration_p[i++] = (raw_data[17] << 8) | raw_data[16];
        _calibration_p[i++] = (raw_data[19] << 8) | raw_data[18];
        _calibration_p[i++] = (raw_data[21] << 8) | raw_data[20];
        _calibration_p[i++] = (raw_data[23] << 8) | raw_data[22];

        i = 0;
        _calibration_h[i++] = raw_data[24];
        _calibration_h[i++] = (raw_data[26] << 8) | raw_data[25];
        _calibration_h[i++] = raw_data[27];
        _calibration_h[i++] = (raw_data[28] << 4) | (0x0F & raw_data[29]);
        _calibration_h[i++] = (raw_data[30] << 4) | ((raw_data[29] >> 4) & 0x0F);
        _calibration_h[i++] = raw_data[31];

        for(i = 1; i < 2; i++) {
                if(_calibration_t[i] & 0x8000)
                        _calibration_t[i] = (-_calibration_t[i] ^ 0xFFFF) + 1;
        }

        for(i = 1; i < 8; i++) {
                if(_calibration_p[i] & 0x8000)
                        _calibration_p[i] = (-_calibration_p[i] ^ 0xFFFF) + 1;

        }

        for(i = 0; i < 6; i++) {
                if(_calibration_h[i] & 0x8000)
                        _calibration_h[i] = (-_calibration_h[i] ^ 0xFFFF) + 1;
        }

        return 0;
}

uint8 _oversampling() {
        return 1 << 7 | 1 << 4;
}

float compensate_temperature(float adc_t) {
        float v1, v2;
        v1 = (adc_t / 16384.0 - _calibration_t[0] / 1024.0) * _calibration_t[1];
        v2 = (adc_t / 131072.0 - _calibration_t[0] / 8192.0) * (adc_t / 131072.0 - _calibration_t[0] / 8192.0) * _calibration_t[2];
        t_fine = v1 + v2;
        return t_fine / 5120.0;
}

float compensate_pressure(float adc_p) {
        float pressure, v1, v2;
        v1 = (t_fine / 2.0) - 64000.0;
        v2 = (((v1 / 4.0) * (v1 / 4.0)) / 2048.0) * _calibration_p[5];
        v2 += ((v1 * _calibration_p[4]) * 2.0);
        v2 = (v2 / 4.0) + (_calibration_p[3] * 65536.0);
        v1 = (((_calibration_p[2] * (((v1 / 4.0) * (v1 / 4.0)) / 8192.0)) / 8.0) + ((_calibration_p[1] * v1) / 2.0)) / 262144.0;
        v1 = ((32768 + v1) * _calibration_p[0]) / 32768.0;
        if (v1 == 0) {
                return 0;
        }

        pressure = ((1048576 - adc_p) - (v2 / 4096.0)) * 3125.0;

        if(pressure < 0x80000000)
                pressure = (pressure * 2.0) / v1;
        else
                pressure = (pressure / v1) * 2.0;

        v1 = (_calibration_p[8] * (((pressure / 8.0) * (pressure / 8.0)) / 8192.0)) / 4096.0;
        v2 = ((pressure / 4.0) * _calibration_p[7]) / 8192.0;
        pressure += ((v1 + v2 + _calibration_p[6]) / 16.0);

        return pressure / 100.0;
}

float compensate_humidity(float adc_h) {
        float var_h = t_fine - 76800.0;
        if(var_h == 0)
                return 0;

        var_h = (adc_h - (_calibration_h[3] * 64.0 + _calibration_h[4] / 16384.0 * var_h)) * (
                _calibration_h[1] / 65536.0 * (1.0 + _calibration_h[5] / 67108864.0 * var_h * (
                                                       1.0 + _calibration_h[2] / 67108864.0 * var_h)));
        var_h *= (1.0 - _calibration_h[0] * var_h / 524288.0);

        if(var_h > 100.0)
                var_h = 100.0;
        else if(var_h < 0.0)
                var_h = 0.0;

        return var_h;
}

/*
   Read data from BME280 and store them to sensor_result struct
 */
void ICACHE_FLASH_ATTR read_all(struct sensor_result* sr) {
        /*
          initialize I2C, see i2c_master.h for details and
          port mappings.
        */
        i2c_init();
        read_calibration_data();
        user_bme280_write_byte(__BME280_CONTROL_HUM, 4);
        user_bme280_write_byte(__BME280_CONTROL, MODE | _oversampling());
        /* wait a bit */
        os_delay_us(20000);
        user_bme280_read_bytes(__BME280_DATA_REG, raw, 8);

        uint16 raw_press[3];
        uint16 raw_temp[3];
        uint16 raw_hum[2];
        double raw_press_f;
        double raw_temp_f;
        double raw_hum_f;
        uint8 i;
        uint8 j;

  #ifdef DEBUG
        for(i = 0; i < 8; i++) {
                os_printf("%s Raw byte from BME: %d %d\n", __FUNCTION__, i, raw[i]);
        }
  #endif


        j = 0;
        for(i = 0; i < 3; i++) {
                raw_press[j++] = raw[i];
        }
        j = 0;
        for(i = 3; i < 6; i++) {
                raw_temp[j++] = raw[i];
        }
        j = 0;
        for(i = 6; i < 8; i++) {
                raw_hum[j++] = raw[i];
        }

        raw_press_f = (raw_press[0] << 12) | (raw_press[1] << 4) | (raw_press[2] >> 4);
        raw_temp_f = (raw_temp[0] << 12) | (raw_temp[1] << 4) | (raw_temp[2] >> 4);
        raw_hum_f = (raw_hum[0] << 8) | raw_hum[1];


        sr->temperature = compensate_temperature(raw_temp_f);
        sr->pressure = compensate_pressure(raw_press_f);
        sr->humidity = compensate_humidity(raw_hum_f);
}
