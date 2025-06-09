# Melopero_BME280_Arduino_Library

The BME280 is made of three sensor: a pressure sensor, a humidity sensor and a temperature sensor.

## How to use

First you need to include the library in your script and create a Melopero_BME280 object:

```C++
#include "Melopero_BME280.h"

Melopero_BME280 bme;

void setup(){
    ...
```

Then before you can retrieve the data from the device you have to initialize it :

```C++
...
void setup(){
    bme.init_device();
    ...
}
```

### Settings

You can select one of three settings presets: weather monitoring, indoor navigation and gaming.

```C++
bme.set_weather_monitoring_configuration();
bme.set_indoor_navigation_configuration();
bme.set_gaming_configuration();
```

Or you can specify your own settings, by specifying the oversampling for each sensor and the filter coefficient:

```C++
// Set the Oversampling:
// To turn off a sensor (and not take measurements from that sensor)
// set the oversampling to BME280_NO_OVERSAMPLING
// The possible oversampling values are :
// BME280.OVERSAMPLING_1X
// BME280.OVERSAMPLING_2X
// BME280.OVERSAMPLING_4X
// BME280.OVERSAMPLING_8X
// BME280.OVERSAMPLING_16X
bme.set_oversampling(pressure_os, temperature_os, humidity_os);

// Set the filter coefficient:
// Possible value for filter coefficient are:
// BME280_FILTER_COEFF_OFF
// BME280_FILTER_COEFF_2
// BME280_FILTER_COEFF_4
// BME280_FILTER_COEFF_8
// BME280_FILTER_COEFF_16
bme.set_filter_coefficient(filter_cofficient);
```

### Get the data

```C++
// Take a measurement and store the result
bme.update_data();

// Get the last measured values
double pressure = bme.get_pressure(); // returns pressure in Pa
double temperature = bme.get_temperature(); // returns temperature in C°
double humidity = bme.get_humidity(); // returns humidity in %
```

### Error codes

Each method of the object (apart from get_pressure, get_temperature and get_humidity) returns a status code. You can check if an error occurred and in case an error occurred what went wrong. Here are all possible error codes:

* BME280_OK
* BME280_E_NULL_PTR
* BME280_E_DEV_NOT_FOUND  
* BME280_E_INVALID_LEN
* BME280_E_COMM_FAIL
* BME280_E_SLEEP_MODE_FAIL
* BME280_E_NVM_COPY_FAILED

Or you can copy this function in your script and input the error codes to print an error message when an error occurs:

```C++
void checkStatus(int8_t status){
  if (status == BME280_OK)
    return;
  else if (status == BME280_E_NULL_PTR)
    Serial.println("error: Null pointer");
  else if (status == BME280_E_DEV_NOT_FOUND)
    Serial.println("error: device not found");
  else if (status == BME280_E_INVALID_LEN)
    Serial.println("error: invalid length");
  else if (status == BME280_E_COMM_FAIL)
    Serial.println("error: communication interface fail");
  else if (status == BME280_E_SLEEP_MODE_FAIL)
    Serial.println("error: sleep mode fail");
  else if (status == BME280_E_NVM_COPY_FAILED)
    Serial.println("error: NVM copy failed");
  else 
    Serial.println("Unknown error");
}
```

**Example:**

```C++
...
int8_t status = BME280_OK;
status = sensor.init_device();
checkStatus(status);
status = sensor.set_indoor_navigation_configuration();
checkStatus(status);
...
```
