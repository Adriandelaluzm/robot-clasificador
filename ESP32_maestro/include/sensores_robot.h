#ifndef SENSORES_ROBOT_H
#define SENSORES_ROBOT_H

#include "Arduino.h"

#define ENCODER_IR_PIN 25
#define LIMIT_DOWN 33
#define LIMIT_UP 32
#define TCS_S0_PIN 27
#define TCS_S1_PIN 26
#define TCS_S2_PIN 14
#define TCS_S3_PIN 13
#define TCS_OUT_PIN 34

enum class BasePosition { HOME = 1, VERDE = 2, ROJO = 3, AZUL = 4 };

extern volatile int marcaCount;

bool limitSwitchDown();
bool limitSwitchUp();
bool encoderAligned(BasePosition targetPos);
void initSensoresRobot();
int contarRanurasIR();
int sensores_robot();
uint8_t detectarColorTCS3200();
const char* nombreColorDetectado(uint8_t color);

#endif
