#include <Arduino.h>
#include "fsm.h"
#include "sensores_robot.h"
#include "i2c_comm.h"

RobotFSM fsm;

void setup() {
  Serial.begin(115200);

  initSensoresRobot();
  i2c_init();
  fsm.init();

  Serial.println("=== CEREBRO ESP32 INICIADO ===");
  Serial.println("Comandos: START, RESET, STATUS");
  Serial.println("En SEARCHING el color se detecta con TCS3200");
}

void loop() {
  fsm.update();
}
