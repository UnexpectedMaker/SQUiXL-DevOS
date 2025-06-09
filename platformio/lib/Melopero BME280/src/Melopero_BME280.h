//Author: Leonardo La Rocca
#ifndef Melopero_BME280_H_INCLUDED
#define Melopero_BME280_H_INCLUDED

#include "api/bme280.h"
#include "Arduino.h"
#include "Wire.h"

/*!
*  @brief Function that creates a mandatory delay required in some of the APIs.
*
* @param[in] period              : Delay in microseconds.
* @param[in, out] intf_ptr       : Void pointer that can enable the linking of descriptors
*                                  for interface related call backs
*  @return void.
*
*/
void delay_us(uint32_t period, void* intf_ptr);



/*!
*  @brief Function for reading the sensor's registers through I2C bus.
*
*  @param[in] reg_addr       : Register address.
*  @param[out] data          : Pointer to the data buffer to store the read data.
*  @param[in] len            : No of bytes to read.
*  @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
*                                  for interface related call backs.
*
*  @return Status of execution
*
*  @retval 0 -> Success
*  @retval > 0 -> Failure Info
*
*/
int8_t i2c_read(uint8_t reg_addr, uint8_t* data, uint32_t len, void* intf_ptr);

/*!
*  @brief Function for writing the sensor's registers through I2C bus.
*
*  @param[in] reg_addr       : Register address.
*  @param[in] data           : Pointer to the data buffer whose value is to be written.
*  @param[in] len            : No of bytes to write.
*  @param[in, out] intf_ptr  : Void pointer that can enable the linking of descriptors
*                                  for interface related call backs
*
*  @return Status of execution
*
*  @retval BME280_OK -> Success
*  @retval BME280_E_COMM_FAIL -> Communication failure.
*
*/
int8_t i2c_write(uint8_t reg_addr, const uint8_t* data, uint32_t len, void* intf_ptr);

struct I2CInterfaceData {
    uint8_t i2c_address;
    TwoWire *i2c;
};

class Melopero_BME280 {

    public:
        struct bme280_dev dev;
        I2CInterfaceData i2c_interface;
        struct bme280_data data;

    public:
        Melopero_BME280();

        int8_t init_device(uint8_t i2c_address = BME280_I2C_ADDR_SEC, TwoWire &bus = Wire);

        int8_t set_oversampling(uint8_t pressure_os, uint8_t temperature_os, uint8_t humidity_os);

        int8_t set_filter_coefficient(uint8_t filter_coefficient);

        int8_t update_data();

        double get_pressure();

        double get_temperature();

        double get_humidity();

        int8_t set_weather_monitoring_configuration();

        int8_t set_indoor_navigation_configuration();

        int8_t set_gaming_configuration();
};

#endif // Melopero_BME280_H_INCLUDED