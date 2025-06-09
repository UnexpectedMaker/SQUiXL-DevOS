#include "Melopero_BME280.h"
#include "Wire.h"

Melopero_BME280 sensor;

void setup() {
  Serial.begin(9600);
  while(!Serial);
  
  Wire.begin();

  int8_t status = BME280_OK;
  status = sensor.init_device(); // sensor.init_device(i2c_address, i2c_bus);
  // use : sensor.init_device(address, Wire1); to use Wire1 instead of Wire.

  checkStatus(status);
  status = sensor.set_indoor_navigation_configuration();
  checkStatus(status);
}

void loop() {
  int8_t status = BME280_OK;
  status = sensor.update_data();
  checkStatus(status);
  printData();
  delay(1000);
}

void printData(){
  Serial.print(sensor.get_temperature());
  Serial.print("°C ");
  Serial.print(sensor.get_pressure());
  Serial.print("Pa ");
  Serial.print(sensor.get_humidity());
  Serial.println("% ");
}

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
