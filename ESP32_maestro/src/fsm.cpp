#include "fsm.h"
#include "sensores_robot.h"

namespace {
constexpr uint8_t COLOR_VERDE = 0;
constexpr uint8_t COLOR_ROJO = 1;
constexpr uint8_t COLOR_AZUL = 2;
constexpr uint8_t COLOR_INVALIDO = 9;

constexpr uint8_t CMD_BASE_ADELANTE = 0x20;
constexpr uint8_t CMD_BASE_STOP = 0x21;
constexpr uint8_t CMD_BASE_ATRAS = 0x22;
constexpr uint8_t CMD_BAJAR = 0x10;
constexpr uint8_t CMD_PINZA_CERRAR = 0x11;
constexpr uint8_t CMD_SUBIR = 0x12;
constexpr uint8_t CMD_PINZA_ABRIR = 0x13;
constexpr uint8_t CMD_PINZA_STOP = 0x14;
constexpr uint8_t CMD_ANTEBRAZO_STOP = 0x15;
// La pinza necesita una ventana real de movimiento antes de enviar STOP.
constexpr unsigned long PINZA_CLOSE_TIME_MS = 800;
constexpr unsigned long PINZA_OPEN_TIME_MS = 800;
constexpr int POSICION_HOME = 1;
constexpr int ULTIMA_POSICION_BASE = 4;

bool baseEnMovimiento = false;
bool baseEnReversa = false;
int posicionActual = POSICION_HOME;
int posicionObjetivo = POSICION_HOME;
State ultimoEstado = State::ERROR;
unsigned long stateStartedAt = 0;
int lecturaAnteriorIR = LOW;
unsigned long ultimoPulso = 0;
unsigned long bloqueoEncoder = 0;
bool encoderArmado = false;

void enviarComando(uint8_t comando) {
  if (!i2cEnviarOrden(comando)) {
    Serial.println("No se pudo entregar el comando por I2C");
  }
}

bool enteredState(State currentState) {
  if (currentState != ultimoEstado) {
    ultimoEstado = currentState;
    stateStartedAt = millis();
    return true;
  }
  return false;
}

bool elapsedInState(unsigned long durationMs) {
  return millis() - stateStartedAt >= durationMs;
}

void sincronizarEncoder() {
  lecturaAnteriorIR = digitalRead(ENCODER_IR_PIN);
  ultimoPulso = millis();
  bloqueoEncoder = millis();
  encoderArmado = (lecturaAnteriorIR == LOW);
}

int posicionObjetivoPorColor(uint8_t color) {
  switch (color) {
    case COLOR_ROJO:
      return 3;
    case COLOR_VERDE:
      return 2;
    case COLOR_AZUL:
      return 4;
    default:
      return -1;
  }
}

const char* nombreColor(uint8_t color) {
  switch (color) {
    case COLOR_VERDE: return "VERDE";
    case COLOR_ROJO: return "ROJO";
    case COLOR_AZUL: return "AZUL";
    default: return "INVALIDO";
  }
}

void actualizarEncoder() {
  const int lecturaActualIR = digitalRead(ENCODER_IR_PIN);

  const int debounceIR = 5;
  const int tiempoBloqueo = 150;

  const bool objetoDetectadoAhora = (lecturaActualIR == HIGH);
  const bool objetoDetectadoAntes = (lecturaAnteriorIR == HIGH);

  // Si arrancamos encima de una marca (HIGH), no contamos nada
  // hasta haber salido primero de esa misma marca.
  if (!encoderArmado) {
    if (!objetoDetectadoAhora) {
      encoderArmado = true;
    }
    lecturaAnteriorIR = lecturaActualIR;
    return;
  }

  if (objetoDetectadoAhora && !objetoDetectadoAntes) {
    if (millis() - ultimoPulso > debounceIR && millis() - bloqueoEncoder > tiempoBloqueo) {
      if (baseEnReversa) {
        posicionActual--;
        if (posicionActual < POSICION_HOME) posicionActual = ULTIMA_POSICION_BASE;
      } else {
        posicionActual++;
        if (posicionActual > ULTIMA_POSICION_BASE) posicionActual = POSICION_HOME;
      }

      bloqueoEncoder = millis();
      ultimoPulso = millis();

      Serial.print("POSICION ACTUAL: ");
      Serial.println(posicionActual);
    }
  }

  lecturaAnteriorIR = lecturaActualIR;
}
}  // namespace

RobotFSM::RobotFSM() : currentState(State::IDLE), detectedColor(COLOR_INVALIDO) {}

void RobotFSM::init() {
  currentState = State::IDLE;
  detectedColor = COLOR_INVALIDO;
  ultimoEstado = State::ERROR;
  stateStartedAt = millis();
  Serial.println("FSM inicializada en IDLE");
}

State RobotFSM::getCurrentState() const {
  return currentState;
}

bool RobotFSM::isDone() const {
  return currentState == State::DONE;
}

void RobotFSM::update() {
  const bool justEntered = enteredState(currentState);

  switch (currentState) {
    case State::IDLE: {
      if (justEntered) {
        baseEnMovimiento = false;
        baseEnReversa = false;
        posicionActual = POSICION_HOME;
        posicionObjetivo = POSICION_HOME;
        detectedColor = COLOR_INVALIDO;
        sincronizarEncoder();
      }

      if (Serial.available() > 0) {
        String comando = Serial.readStringUntil('\n');
        comando.trim();

        if (comando == "START" || comando == "start" || comando == "s") {
          Serial.println("INICIANDO SECUENCIA...");
          currentState = State::SEARCHING;
        } else if (comando == "RESET") {
          currentState = State::IDLE;
          Serial.println("FSM RESET");
        } else if (comando == "STATUS") {
          Serial.printf("Estado actual: %d\n", static_cast<int>(currentState));
          Serial.printf("Limit DOWN=%d, Limit UP=%d, Encoder=%d\n",
                        ::limitSwitchDown(), ::limitSwitchUp(),
                        digitalRead(ENCODER_IR_PIN));
        }
      }
      break;
    }

    case State::SEARCHING:
      if (justEntered) {
        Serial.println("SEARCHING");
        Serial.println("Detectando color con TCS3200...");
      }
      if (Serial.available() > 0) {
        String comando = Serial.readStringUntil('\n');
        comando.trim();

        if (comando == "RESET") {
          currentState = State::IDLE;
          Serial.println("FSM RESET");
          break;
        }
        if (comando == "STATUS") {
          Serial.printf("Estado actual: %d\n", static_cast<int>(currentState));
          Serial.println("Intentando detectar color con TCS3200");
        }
      }

      detectedColor = detectarColorTCS3200();
      if (detectedColor == COLOR_INVALIDO) {
        if (justEntered) {
          Serial.println("No se pudo identificar un color valido");
        }
      } else {
        Serial.printf("Color detectado: %s (%u)\n", nombreColor(detectedColor), detectedColor);
        currentState = State::MOVING_DOWN;
      }
      break;

    case State::MOVING_DOWN:
      if (justEntered) {
        Serial.printf("Entrando a MOVING_DOWN. Limit DOWN=%d, Limit UP=%d\n", ::limitSwitchDown(), ::limitSwitchUp());
        enviarComando(CMD_BAJAR);
      }
      if (::limitSwitchDown()) {
        enviarComando(CMD_ANTEBRAZO_STOP);
        Serial.println("Limit switch DOWN activo");
        currentState = State::MOVING_PINZA_CLOSE;
        Serial.println("MOVING_PINZA_CLOSE");
      }
      break;

    case State::MOVING_PINZA_CLOSE:
      if (justEntered) enviarComando(CMD_PINZA_CERRAR);
      if (elapsedInState(PINZA_CLOSE_TIME_MS)) {
        enviarComando(CMD_PINZA_STOP);
        currentState = State::MOVING_UP;
        Serial.println("MOVING_UP");
      }
      break;

    case State::MOVING_UP:
      if (justEntered) {
        Serial.printf("Entrando a MOVING_UP. Limit DOWN=%d, Limit UP=%d\n", ::limitSwitchDown(), ::limitSwitchUp());
        enviarComando(CMD_SUBIR);
      }
      if (::limitSwitchUp()) {
        enviarComando(CMD_ANTEBRAZO_STOP);
        Serial.println("Limit switch UP activo");
        currentState = State::MOVING_GIRAR_BASE;
        baseEnMovimiento = false;
        posicionActual = POSICION_HOME;
        posicionObjetivo = POSICION_HOME;
        Serial.println("MOVING_GIRAR_BASE");
      }
      break;

    case State::MOVING_GIRAR_BASE:
      if (!baseEnMovimiento) {
        posicionObjetivo = posicionObjetivoPorColor(detectedColor);
        if (posicionObjetivo < 0) {
          Serial.println("Color no valido para mover base");
          currentState = State::ERROR;
          break;
        }
        sincronizarEncoder();
        enviarComando(CMD_BASE_ADELANTE);
        baseEnMovimiento = true;
        baseEnReversa = false;
        Serial.printf("Moviendo base segun color %s a posicion %d\n", nombreColor(detectedColor), posicionObjetivo);
      }
      actualizarEncoder();
      if (posicionActual == posicionObjetivo) {
        enviarComando(CMD_BASE_STOP);
        baseEnMovimiento = false;
        currentState = State::MOVING_DOWN_2;
        Serial.println("MOVING_DOWN_2");
      }
      break;

    case State::MOVING_DOWN_2:
      if (justEntered) {
        Serial.printf("Entrando a MOVING_DOWN_2. Limit DOWN=%d, Limit UP=%d\n", ::limitSwitchDown(), ::limitSwitchUp());
        enviarComando(CMD_BAJAR);
      }
      if (::limitSwitchDown()) {
        enviarComando(CMD_ANTEBRAZO_STOP);
        Serial.println("Limit switch DOWN activo");
        currentState = State::MOVING_PINZA_OPEN;
        Serial.println("MOVING_PINZA_OPEN");
      }
      break;

    case State::MOVING_PINZA_OPEN:
      if (justEntered) enviarComando(CMD_PINZA_ABRIR);
      if (elapsedInState(PINZA_OPEN_TIME_MS)) {
        enviarComando(CMD_PINZA_STOP);
        currentState = State::MOVING_UP_2;
        Serial.println("MOVING_UP_2");
      }
      break;

    case State::MOVING_UP_2:
      if (justEntered) {
        Serial.printf("Entrando a MOVING_UP_2. Limit DOWN=%d, Limit UP=%d\n", ::limitSwitchDown(), ::limitSwitchUp());
        enviarComando(CMD_SUBIR);
      }
      if (::limitSwitchUp()) {
        enviarComando(CMD_ANTEBRAZO_STOP);
        Serial.println("Limit switch UP activo");
        currentState = State::MOVING_HOME;
        baseEnMovimiento = false;
        posicionObjetivo = POSICION_HOME;
        Serial.println("MOVING_HOME");
      }
      break;

    case State::MOVING_HOME:
      if (!baseEnMovimiento) {
        posicionObjetivo = POSICION_HOME;
        sincronizarEncoder();
        enviarComando(CMD_BASE_ATRAS);
        baseEnMovimiento = true;
        baseEnReversa = true;
        Serial.println("Regresando base a HOME");
      }
      actualizarEncoder();
      if (posicionActual == posicionObjetivo) {
        enviarComando(CMD_BASE_STOP);
        baseEnMovimiento = false;
        baseEnReversa = false;
        Serial.println("HOME alcanzado");
        currentState = State::DONE;
      }
      break;

    case State::DONE:
      if (justEntered) Serial.println("SECUENCIA COMPLETA!");
      currentState = State::IDLE;
      break;

    case State::ERROR:
      if (justEntered) Serial.println("ERROR - Reiniciando...");
      if (elapsedInState(2000)) currentState = State::IDLE;
      break;
  }
}
