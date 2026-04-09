// i2c_comm.h
#ifndef I2C_COMM_H
#define I2C_COMM_H

#include "Arduino.h"
#include <Wire.h>

void i2c_init();
bool i2cEnviarOrden(uint8_t orden);
bool i2cRecibidoACK();

#endif
