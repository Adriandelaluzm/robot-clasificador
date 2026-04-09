# Reporte de funcionamiento del proyecto ESP32_maestro

## 1. Objetivo del proyecto

Este proyecto implementa el "cerebro" de un sistema robótico basado en **ESP32**. Su función principal es:

- Leer sensores del robot.
- Detectar el color de un objeto con un sensor **TCS3200**.
- Ejecutar una secuencia automática de toma y liberación del objeto.
- Enviar órdenes por **I2C** a un controlador externo encargado de mover actuadores como base, antebrazo y pinza.

En conjunto, el ESP32 no mueve directamente los motores de potencia, sino que **coordina el proceso completo** mediante una **máquina de estados finitos (FSM)**.

## 2. Estructura del proyecto

Los módulos principales son:

- `src/main.cpp`: arranque del sistema y ciclo principal.
- `src/fsm.cpp`: lógica principal de control mediante máquina de estados.
- `src/sensores_robot.cpp`: lectura de finales de carrera, encoder IR y sensor de color TCS3200.
- `src/i2c_comm.cpp`: comunicación I2C con el dispositivo esclavo.
- `include/*.h`: declaraciones de estados, pines, funciones y estructuras de apoyo.

## 3. Plataforma y configuración

Según `platformio.ini`, el proyecto usa:

- **Plataforma**: `espressif32`
- **Tarjeta**: `esp32dev`
- **Framework**: `arduino`
- **Velocidad del monitor serial**: `115200`

También incluye la librería `Wire` para I2C y `ArduinoJson`, aunque en el código revisado la serialización JSON no se está usando de forma activa.

## 4. Hardware involucrado

### ESP32

Es el nodo maestro del sistema. Inicializa sensores, habilita la comunicación I2C y ejecuta continuamente la FSM.

### Sensor de color TCS3200

Se usa para identificar el color del objeto antes de mover la base hacia el destino correspondiente.

Pines definidos:

- `TCS_S0_PIN = 27`
- `TCS_S1_PIN = 26`
- `TCS_S2_PIN = 14`
- `TCS_S3_PIN = 13`
- `TCS_OUT_PIN = 34`

### Encoder / sensor IR de posición

Se usa para contar marcas y estimar la posición angular de la base.

- `ENCODER_IR_PIN = 25`

### Finales de carrera

Permiten detectar los extremos del movimiento vertical:

- `LIMIT_DOWN = 33`
- `LIMIT_UP = 32`

## 5. Principio general de funcionamiento

El firmware sigue esta lógica:

1. Espera en reposo (`IDLE`).
2. Al recibir `START` por el monitor serial, inicia la secuencia.
3. Detecta el color del objeto.
4. Baja el mecanismo.
5. Cierra la pinza para sujetar el objeto.
6. Sube el mecanismo.
7. Gira la base hasta la posición asociada al color.
8. Baja nuevamente.
9. Abre la pinza para soltar el objeto.
10. Sube otra vez.
11. Regresa la base a posición HOME.
12. Finaliza y vuelve al estado de espera.

## 6. Máquina de estados (FSM)

La FSM está definida en `include/fsm.h` y ejecutada en `src/fsm.cpp`.

Estados implementados:

- `IDLE`
- `SEARCHING`
- `MOVING_DOWN`
- `MOVING_PINZA_CLOSE`
- `MOVING_UP`
- `MOVING_GIRAR_BASE`
- `MOVING_DOWN_2`
- `MOVING_PINZA_OPEN`
- `MOVING_UP_2`
- `MOVING_HOME`
- `DONE`
- `ERROR`

### Descripción por estado

#### `IDLE`

Estado de reposo. Aquí el sistema:

- Reinicia variables internas.
- Sincroniza la lectura inicial del encoder.
- Espera comandos por puerto serial.

Comandos admitidos:

- `START`, `start` o `s`: inicia la secuencia.
- `RESET`: reinicia la FSM.
- `STATUS`: imprime estado y lecturas básicas.

#### `SEARCHING`

El sistema intenta detectar el color del objeto con el TCS3200.

- Si no detecta un color válido, permanece intentando.
- Si identifica un color, pasa a `MOVING_DOWN`.

Mapeo de colores:

- `VERDE = 0`
- `ROJO = 1`
- `AZUL = 2`
- `INVALIDO = 9`

#### `MOVING_DOWN`

Envía por I2C el comando de bajar el mecanismo.

- Comando: `0x10`
- Cuando el final de carrera inferior se activa, detiene el antebrazo y pasa a cerrar la pinza.

#### `MOVING_PINZA_CLOSE`

Activa el cierre de la pinza durante una ventana fija de tiempo.

- Comando cerrar: `0x11`
- Tiempo de cierre: `800 ms`
- Comando stop pinza: `0x14`

Después pasa a `MOVING_UP`.

#### `MOVING_UP`

Ordena subir el mecanismo:

- Comando subir: `0x12`
- Cuando se activa el final de carrera superior, detiene el antebrazo (`0x15`) y pasa a girar la base.

#### `MOVING_GIRAR_BASE`

Mueve la base a la posición definida por el color detectado:

- Base adelante: `0x20`
- Base stop: `0x21`

Posiciones destino:

- HOME = `1`
- VERDE = `2`
- ROJO = `3`
- AZUL = `4`

El avance se controla contando marcas del encoder IR. Cuando `posicionActual` coincide con `posicionObjetivo`, la base se detiene y el sistema continúa.

#### `MOVING_DOWN_2`

Baja nuevamente el mecanismo en la posición destino para preparar la liberación del objeto.

#### `MOVING_PINZA_OPEN`

Abre la pinza durante un tiempo fijo:

- Comando abrir: `0x13`
- Tiempo de apertura: `800 ms`
- Stop pinza: `0x14`

Después pasa a `MOVING_UP_2`.

#### `MOVING_UP_2`

Eleva nuevamente el mecanismo hasta activar el final de carrera superior y luego se prepara para regresar a HOME.

#### `MOVING_HOME`

Regresa la base a la posición inicial:

- Base atrás: `0x22`
- Base stop: `0x21`

El regreso también se controla con el encoder IR.

#### `DONE`

Indica que la secuencia terminó correctamente. Luego el sistema vuelve a `IDLE`.

#### `ERROR`

Se usa cuando ocurre una condición inválida, por ejemplo si el color detectado no tiene una posición asociada. Tras un breve tiempo, el sistema regresa a `IDLE`.

## 7. Comunicación I2C

La comunicación I2C está en `src/i2c_comm.cpp`.

Características observadas:

- El ESP32 actúa como maestro.
- La dirección del esclavo es `0x08`.
- Cada acción del robot se representa con un byte de comando.

Comandos identificados:

- `0x10`: bajar
- `0x11`: cerrar pinza
- `0x12`: subir
- `0x13`: abrir pinza
- `0x14`: detener pinza
- `0x15`: detener antebrazo
- `0x20`: base adelante
- `0x21`: base stop
- `0x22`: base atrás

El sistema reporta por serial si la transmisión fue correcta o si hubo error de I2C.

## 8. Lectura de sensores

### Finales de carrera

Las funciones:

- `limitSwitchDown()`
- `limitSwitchUp()`

consideran activo el sensor cuando la lectura digital es `LOW`, lo que sugiere conexión con `INPUT_PULLUP`.

### Encoder IR de base

El proyecto usa transiciones del sensor IR para actualizar la posición de la base.

Aspectos importantes:

- Hay anti-rebote por tiempo.
- Existe una lógica para evitar contar una marca si el sistema arrancó ya colocado sobre ella.
- El conteo incrementa o decrementa según la dirección de movimiento de la base.
- La posición se considera circular entre `1` y `4`.

### Sensor TCS3200

La detección de color se realiza midiendo el ancho de pulso de salida del TCS3200 para los filtros rojo, verde y azul.

La lógica aplicada es:

- Se toman 10 muestras por color.
- Se promedian los pulsos.
- Un pulso menor significa mayor presencia de ese color.
- Si las lecturas son demasiado parecidas, el sistema interpreta que no hay objeto o que el fondo no es distinguible.
- Si no se alcanza una separación mínima entre componentes, el color se marca como inválido.

## 9. Flujo de operación completo

El comportamiento esperado del sistema es:

1. Encender el ESP32.
2. Inicializar sensores, I2C y FSM.
3. Esperar el comando `START`.
4. Detectar el color del objeto.
5. Tomar el objeto con movimiento vertical y cierre de pinza.
6. Llevar el objeto a la estación correspondiente al color.
7. Soltar el objeto.
8. Volver a posición inicial.
9. Quedar listo para un nuevo ciclo.

## 10. Entradas y salidas del sistema

### Entradas

- Comandos por puerto serial.
- Sensor de color TCS3200.
- Final de carrera superior.
- Final de carrera inferior.
- Sensor IR para referencia de posición.

### Salidas

- Bytes de comando por I2C al controlador esclavo.
- Mensajes de depuración por monitor serial.

## 11. Fortalezas del diseño actual

- La lógica está separada por módulos.
- La FSM hace el comportamiento fácil de seguir y depurar.
- El uso de finales de carrera mejora la seguridad del movimiento vertical.
- El posicionamiento por color está claramente definido.
- Hay mensajes seriales suficientes para observar el ciclo de ejecución.

## 12. Observaciones técnicas y riesgos detectados

- La función `i2cRecibidoACK()` siempre devuelve `true`, por lo que no valida una confirmación real del esclavo.
- La detección de color depende de umbrales fijos; puede requerir calibración según iluminación, distancia y material del objeto.
- El cierre y apertura de la pinza están controlados por tiempo fijo, no por realimentación mecánica.
- La lógica de posicionamiento de la base asume cuatro posiciones discretas y una secuencia circular estable.
- Si el encoder pierde pulsos por ruido o mala alineación, la base podría detenerse en una posición incorrecta.
- En `SEARCHING`, si no se detecta un color válido, el sistema sigue intentando indefinidamente hasta recibir un color reconocible o un `RESET`.

## 13. Conclusión

El proyecto implementa correctamente una arquitectura de control para un robot clasificador simple. El **ESP32** funciona como maestro de decisión, mientras que el movimiento físico queda delegado a un esclavo por **I2C**. La lógica principal está basada en una FSM robusta y fácil de ampliar.

En su estado actual, el sistema está diseñado para:

- identificar un objeto por color,
- recogerlo,
- trasladarlo a una posición asociada,
- liberarlo,
- y regresar automáticamente al punto de inicio.

Como siguiente etapa de mejora, sería recomendable incorporar:

- confirmación real del esclavo I2C,
- calibración formal del TCS3200,
- validación de tiempo máximo por estado,
- y retroalimentación adicional para la pinza y la base.
