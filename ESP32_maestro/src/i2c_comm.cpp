// src/i2c_comm.cpp
#include "i2c_comm.h"

const uint8_t ARDUINO_ADDR = 0x08;

void i2c_init() {
  Wire.begin();
  Serial.printf("I2C inicializado. SDA=%d SCL=%d Addr=0x%02X\n", SDA, SCL, ARDUINO_ADDR);
}

bool i2cEnviarOrden(uint8_t orden) {
  Wire.beginTransmission(ARDUINO_ADDR);
  Wire.write(orden);
  const uint8_t result = Wire.endTransmission();

  if (result == 0) {
    Serial.printf("I2C OK -> comando 0x%02X\n", orden);
    return true;
  }

  Serial.printf("I2C ERROR (%u) -> comando 0x%02X\n", result, orden);
  return false;
}

bool i2cRecibidoACK() {
  return true;
}
