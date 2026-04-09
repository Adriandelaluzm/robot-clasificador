#include <Arduino.h>
#include <AFMotor.h>
#include <Servo.h>
#include <Wire.h>

constexpr uint8_t I2C_ADDR = 0x08;

constexpr uint8_t CMD_SERVO_RECOGE = 0x01;
constexpr uint8_t CMD_SERVO_LECTURA = 0x02;
constexpr uint8_t CMD_SERVO_TIRAR = 0x03;
constexpr uint8_t CMD_BAJAR = 0x10;
constexpr uint8_t CMD_PINZA_CERRAR = 0x11;
constexpr uint8_t CMD_SUBIR = 0x12;
constexpr uint8_t CMD_PINZA_ABRIR = 0x13;
constexpr uint8_t CMD_PINZA_STOP = 0x14;
constexpr uint8_t CMD_ANTEBRAZO_STOP = 0x15;
constexpr uint8_t CMD_BASE_ADELANTE = 0x20;
constexpr uint8_t CMD_BASE_STOP = 0x21;
constexpr uint8_t CMD_BASE_ATRAS = 0x22;

constexpr uint8_t PIN_SERVO_CANICA = 9;

constexpr uint8_t MOTOR_BASE = 1;
constexpr uint8_t MOTOR_ANTEBRAZO = 2;
constexpr uint8_t MOTOR_PINZA = 3;

constexpr uint8_t VELOCIDAD_BASE = 150;
constexpr uint8_t VELOCIDAD_ANTEBRAZO = 150;
constexpr uint8_t VELOCIDAD_PINZA = 150;
constexpr int ANGULO_RECOGE = 180;
constexpr int ANGULO_LECTURA = 130;
constexpr int ANGULO_TIRAR = 85;

Servo servoCanica;
AF_DCMotor motorBase(MOTOR_BASE);
AF_DCMotor motorAntebrazo(MOTOR_ANTEBRAZO);
AF_DCMotor motorPinza(MOTOR_PINZA);

volatile uint8_t ultimoComando = 0;
volatile bool comandoPendiente = false;

void detenerAntebrazo() {
  motorAntebrazo.run(RELEASE);
}

void antebrazoBajar() {
  motorAntebrazo.run(BACKWARD);
}

void antebrazoSubir() {
  motorAntebrazo.run(FORWARD);
}

void baseAdelante() {
  motorBase.run(FORWARD);
}

void baseAtras() {
  motorBase.run(BACKWARD);
}

void baseStop() {
  motorBase.run(RELEASE);
}

void pinzaCerrar() {
  motorPinza.run(FORWARD);
}

void pinzaAbrir() {
  motorPinza.run(BACKWARD);
}

void pinzaStop() {
  motorPinza.run(RELEASE);
}

void ejecutarComando(uint8_t comando) {
  switch (comando) {
    case CMD_SERVO_RECOGE:
      servoCanica.write(ANGULO_RECOGE);
      Serial.println(F("CMD_SERVO_RECOGE"));
      break;
    case CMD_SERVO_LECTURA:
      servoCanica.write(ANGULO_LECTURA);
      Serial.println(F("CMD_SERVO_LECTURA"));
      break;
    case CMD_SERVO_TIRAR:
      servoCanica.write(ANGULO_TIRAR);
      Serial.println(F("CMD_SERVO_TIRAR"));
      break;
    case CMD_BAJAR:
      antebrazoBajar();
      Serial.println(F("CMD_BAJAR"));
      break;
    case CMD_SUBIR:
      antebrazoSubir();
      Serial.println(F("CMD_SUBIR"));
      break;
    case CMD_ANTEBRAZO_STOP:
      detenerAntebrazo();
      Serial.println(F("CMD_ANTEBRAZO_STOP"));
      break;
    case CMD_PINZA_CERRAR:
      pinzaCerrar();
      Serial.println(F("CMD_PINZA_CERRAR"));
      break;
    case CMD_PINZA_ABRIR:
      pinzaAbrir();
      Serial.println(F("CMD_PINZA_ABRIR"));
      break;
    case CMD_PINZA_STOP:
      pinzaStop();
      Serial.println(F("CMD_PINZA_STOP"));
      break;
    case CMD_BASE_ADELANTE:
      baseAdelante();
      Serial.println(F("CMD_BASE_ADELANTE"));
      break;
    case CMD_BASE_ATRAS:
      baseAtras();
      Serial.println(F("CMD_BASE_ATRAS"));
      break;
    case CMD_BASE_STOP:
      baseStop();
      Serial.println(F("CMD_BASE_STOP"));
      break;
    default:
      Serial.print(F("Comando desconocido: 0x"));
      Serial.println(comando, HEX);
      break;
  }
}

void onReceiveI2C(int bytesRecibidos) {
  (void)bytesRecibidos;
  while (Wire.available()) {
    ultimoComando = Wire.read();
    comandoPendiente = true;
  }
}

void onRequestI2C() {
  Wire.write(1);
}

void setup() {
  Serial.begin(115200);

  motorBase.setSpeed(VELOCIDAD_BASE);
  motorAntebrazo.setSpeed(VELOCIDAD_ANTEBRAZO);
  motorPinza.setSpeed(VELOCIDAD_PINZA);

  detenerAntebrazo();
  baseStop();
  pinzaStop();

  servoCanica.attach(PIN_SERVO_CANICA);
  servoCanica.write(ANGULO_LECTURA);

  Wire.begin(I2C_ADDR);
  Wire.onReceive(onReceiveI2C);
  Wire.onRequest(onRequestI2C);

  Serial.println(F("Arduino esclavo I2C listo"));
}

void loop() {
  if (comandoPendiente) {
    noInterrupts();
    const uint8_t comando = ultimoComando;
    comandoPendiente = false;
    interrupts();

    ejecutarComando(comando);
  }

}
