#ifndef FSM_H
#define FSM_H

#include "Arduino.h"
#include "i2c_comm.h"

enum class State {
  IDLE,
  SEARCHING,
  MOVING_DOWN,
  MOVING_PINZA_CLOSE,
  MOVING_UP,
  MOVING_GIRAR_BASE,
  MOVING_DOWN_2,
  MOVING_PINZA_OPEN,
  MOVING_UP_2,
  MOVING_HOME,
  DONE,
  ERROR
};

class RobotFSM {
private:
  State currentState;
  uint8_t detectedColor;

public:
  RobotFSM();
  void init();
  void update();
  State getCurrentState() const;
  bool isDone() const;
};

#endif
