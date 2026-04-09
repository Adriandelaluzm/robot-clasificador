#include "sensores_robot.h"

volatile int marcaCount = 0;
static volatile int ranurasContadas = 0;
static unsigned long ultimaRanura = 0;

namespace {
constexpr uint8_t COLOR_VERDE = 0;
constexpr uint8_t COLOR_ROJO = 1;
constexpr uint8_t COLOR_AZUL = 2;
constexpr uint8_t COLOR_INVALIDO = 9;
constexpr unsigned long TCS_TIMEOUT_US = 30000;
constexpr int TCS_MUESTRAS = 10;
constexpr long TCS_MIN_SEPARACION_COLOR = 15;

unsigned long leerPulsoColor(bool s2, bool s3) {
  digitalWrite(TCS_S2_PIN, s2 ? HIGH : LOW);
  digitalWrite(TCS_S3_PIN, s3 ? HIGH : LOW);
  delay(2);

  return pulseIn(TCS_OUT_PIN, LOW, TCS_TIMEOUT_US);
}
}  // namespace

void initSensoresRobot() {
  pinMode(ENCODER_IR_PIN, INPUT);
  pinMode(LIMIT_DOWN, INPUT_PULLUP);
  pinMode(LIMIT_UP, INPUT_PULLUP);
  pinMode(TCS_S0_PIN, OUTPUT);
  pinMode(TCS_S1_PIN, OUTPUT);
  pinMode(TCS_S2_PIN, OUTPUT);
  pinMode(TCS_S3_PIN, OUTPUT);
  pinMode(TCS_OUT_PIN, INPUT);

  digitalWrite(TCS_S0_PIN, HIGH);
  digitalWrite(TCS_S1_PIN, LOW);
  digitalWrite(TCS_S2_PIN, LOW);
  digitalWrite(TCS_S3_PIN, LOW);

  ranurasContadas = 0;
  ultimaRanura = millis();

  Serial.println("Sensores del robot inicializados");
}

bool limitSwitchDown() {
  return digitalRead(LIMIT_DOWN) == LOW;
}

bool limitSwitchUp() {
  return digitalRead(LIMIT_UP) == LOW;
}

int contarRanurasIR() {
  static bool ultimaLectura = LOW;
  const bool lecturaActual = digitalRead(ENCODER_IR_PIN);

  if (ultimaLectura == LOW && lecturaActual == HIGH) {
    ranurasContadas++;
    marcaCount = ranurasContadas;
    ultimaRanura = millis();
    Serial.printf("Ranura #%d detectada\n", ranurasContadas);
  }

  ultimaLectura = lecturaActual;
  return ranurasContadas;
}

bool encoderAligned(BasePosition targetPos) {
  const int ranurasObjetivo = static_cast<int>(targetPos);

  if (ranurasContadas >= ranurasObjetivo) {
    Serial.printf("BASE ALINEADA: %d/%d\n", ranurasContadas, ranurasObjetivo);
    ranurasContadas = 0;
    marcaCount = 0;
    return true;
  }

  if (millis() - ultimaRanura > 5000) {
    Serial.println("TIMEOUT encoder");
    ranurasContadas = 0;
    marcaCount = 0;
    return false;
  }

  return false;
}

int sensores_robot() {
  return contarRanurasIR();
}

uint8_t detectarColorTCS3200() {
  unsigned long acumuladoRojo = 0;
  unsigned long acumuladoVerde = 0;
  unsigned long acumuladoAzul = 0;

  for (int i = 0; i < TCS_MUESTRAS; ++i) {
    acumuladoRojo += leerPulsoColor(LOW, LOW);
    acumuladoAzul += leerPulsoColor(LOW, HIGH);
    acumuladoVerde += leerPulsoColor(HIGH, HIGH);
  }

  const unsigned long pulsoRojo = acumuladoRojo / TCS_MUESTRAS;
  const unsigned long pulsoAzul = acumuladoAzul / TCS_MUESTRAS;
  const unsigned long pulsoVerde = acumuladoVerde / TCS_MUESTRAS;
  const long diferenciaRG = abs(static_cast<long>(pulsoRojo) - static_cast<long>(pulsoVerde));
  const long diferenciaRB = static_cast<long>(pulsoRojo) - static_cast<long>(pulsoAzul);
  const long diferenciaGB = static_cast<long>(pulsoVerde) - static_cast<long>(pulsoAzul);
  const unsigned long menorPulso = min(pulsoRojo, min(pulsoVerde, pulsoAzul));
  const unsigned long mayorPulso = max(pulsoRojo, max(pulsoVerde, pulsoAzul));

  Serial.printf("TCS3200 R=%lu G=%lu B=%lu\n", pulsoRojo, pulsoVerde, pulsoAzul);

  if (pulsoRojo == 0 || pulsoVerde == 0 || pulsoAzul == 0) {
    return COLOR_INVALIDO;
  }

  Serial.printf("TCS3200 dRB=%ld dGB=%ld\n", diferenciaRB, diferenciaGB);

  // Blanco/fondo: lecturas muy parecidas entre si.
  if ((mayorPulso - menorPulso) < TCS_MIN_SEPARACION_COLOR) {
    Serial.println("TCS3200 sin objeto o fondo");
    return COLOR_INVALIDO;
  }

  // Con esta configuracion, un pulso menor significa mayor presencia de ese color.
  if (pulsoVerde < pulsoRojo && pulsoVerde < pulsoAzul &&
      (static_cast<long>(pulsoRojo) - static_cast<long>(pulsoVerde)) >= TCS_MIN_SEPARACION_COLOR &&
      (static_cast<long>(pulsoAzul) - static_cast<long>(pulsoVerde)) >= TCS_MIN_SEPARACION_COLOR) {
    return COLOR_VERDE;
  }

  if (pulsoRojo < pulsoVerde && pulsoRojo < pulsoAzul &&
      (static_cast<long>(pulsoVerde) - static_cast<long>(pulsoRojo)) >= TCS_MIN_SEPARACION_COLOR &&
      (static_cast<long>(pulsoAzul) - static_cast<long>(pulsoRojo)) >= TCS_MIN_SEPARACION_COLOR) {
    return COLOR_ROJO;
  }

  if (pulsoAzul < pulsoRojo && pulsoAzul < pulsoVerde &&
      (static_cast<long>(pulsoRojo) - static_cast<long>(pulsoAzul)) >= TCS_MIN_SEPARACION_COLOR &&
      (static_cast<long>(pulsoVerde) - static_cast<long>(pulsoAzul)) >= TCS_MIN_SEPARACION_COLOR) {
    return COLOR_AZUL;
  }

  Serial.println("TCS3200 color indeterminado");
  return COLOR_INVALIDO;
}

const char* nombreColorDetectado(uint8_t color) {
  switch (color) {
    case COLOR_VERDE: return "VERDE";
    case COLOR_ROJO: return "ROJO";
    case COLOR_AZUL: return "AZUL";
    default: return "INVALIDO";
  }
}
