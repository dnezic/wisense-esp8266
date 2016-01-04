#include "debug.h"

#define BME280_addr    0x76
#define MODE 1

#define __BME280_SLEEP 0
#define __BME280_FORCED 1
#define __BME280_NORMAL 3
#define __BME280_CONTROL 0xF4  // Control measurement
#define __BME280_CONTROL_HUM 0xF2  // Control humidity
#define __BME280_TEMPDATA 0xFA  // Temperature
#define __BME280_PRESSURE_DATA 0xF7  // Pressure
#define __BME280_HUMIDITY_DATA 0xFD  // Humidity
#define __BME280_STATUS 0xF3  // Status
#define __BME280_DATA_REG 0xF7  // Start of data registers (support shadowing)

struct sensor_result {
    float temperature;
    float pressure;
    float humidity;
};
