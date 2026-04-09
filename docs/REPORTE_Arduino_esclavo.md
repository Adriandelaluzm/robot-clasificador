# Reporte de funcionamiento del proyecto

## 1. Resumen general

El proyecto implementa un **Arduino Uno como esclavo I2C** para controlar un sistema electromecánico compuesto por:

- Un servo para posicionar una canica en distintos ángulos.
- Un motor DC para la base.
- Un motor DC para el antebrazo.
- Un motor DC para la pinza.

La lógica principal recibe comandos por I2C, los almacena temporalmente y los ejecuta en el `loop()` principal. Adicionalmente, el sistema responde con un byte fijo cuando otro dispositivo solicita datos por I2C.

## 2. Plataforma y dependencias

Según [platformio.ini](C:\Users\jadri\OneDrive\Documentos\PlatformIO\Projects\Arduino_esclavo\platformio.ini), el proyecto usa:

- Plataforma: `atmelavr`
- Tarjeta: `uno`
- Framework: `arduino`
- Librerías:
  - `Adafruit Motor Shield library`
  - `Servo`

## 3. Archivo principal analizado

La funcionalidad está concentrada en [src/main.cpp](C:\Users\jadri\OneDrive\Documentos\PlatformIO\Projects\Arduino_esclavo\src\main.cpp).

### 3.1 Configuración de comunicación

- Dirección I2C del esclavo: `0x08`
- Velocidad del monitor serial: `115200`
- Evento de recepción I2C: `Wire.onReceive(onReceiveI2C)`
- Evento de respuesta I2C: `Wire.onRequest(onRequestI2C)`

Cuando el maestro I2C envía datos, el Arduino lee todos los bytes recibidos pero conserva como comando efectivo **el último byte leído**.

Cuando el maestro solicita una respuesta, el Arduino envía siempre el valor `1`.

## 4. Actuadores controlados

### 4.1 Servo de canica

- Pin del servo: `9`
- Posiciones definidas:
  - `ANGULO_RECOGE = 180`
  - `ANGULO_LECTURA = 130`
  - `ANGULO_TIRAR = 85`

Al iniciar, el servo se coloca en la posición de lectura (`130`).

### 4.2 Motores DC

Asignación de motores en el Motor Shield:

- Base: motor `1`
- Antebrazo: motor `2`
- Pinza: motor `3`

Velocidades configuradas:

- Base: `150`
- Antebrazo: `150`
- Pinza: `150`

Durante `setup()`, los tres actuadores quedan inicialmente detenidos.

## 5. Mapa de comandos

Los comandos definidos y su comportamiento son:

| Comando | Valor hexadecimal | Acción |
| --- | --- | --- |
| `CMD_SERVO_RECOGE` | `0x01` | Mueve el servo a `180` |
| `CMD_SERVO_LECTURA` | `0x02` | Mueve el servo a `130` |
| `CMD_SERVO_TIRAR` | `0x03` | Mueve el servo a `85` |
| `CMD_BAJAR` | `0x10` | Hace bajar el antebrazo |
| `CMD_PINZA_CERRAR` | `0x11` | Cierra la pinza |
| `CMD_SUBIR` | `0x12` | Hace subir el antebrazo |
| `CMD_PINZA_ABRIR` | `0x13` | Abre la pinza |
| `CMD_PINZA_STOP` | `0x14` | Detiene la pinza |
| `CMD_ANTEBRAZO_STOP` | `0x15` | Detiene el antebrazo |
| `CMD_BASE_ADELANTE` | `0x20` | Mueve la base hacia adelante |
| `CMD_BASE_STOP` | `0x21` | Detiene la base |
| `CMD_BASE_ATRAS` | `0x22` | Mueve la base hacia atrás |

Cada vez que se ejecuta un comando válido, el sistema imprime un mensaje descriptivo por el puerto serial. Si el comando no existe, imprime el valor hexadecimal recibido.

## 6. Flujo de funcionamiento

El flujo operativo del firmware es el siguiente:

1. Se inicializa el puerto serial.
2. Se configuran las velocidades de los motores.
3. Se detienen base, antebrazo y pinza.
4. Se conecta el servo y se coloca en posición de lectura.
5. Se inicia el bus I2C como esclavo en la dirección `0x08`.
6. Cuando llega un comando por I2C, este se guarda en variables compartidas.
7. En el `loop()`, si hay un comando pendiente, se copia de forma segura y se ejecuta fuera de la rutina de interrupción.

Este diseño es correcto para evitar ejecutar acciones de hardware pesadas dentro del callback de I2C.

## 7. Estado funcional observado

Con base en el código fuente, el proyecto **sí implementa correctamente la lógica de recepción y ejecución de comandos** para los actuadores declarados. El sistema también tiene una inicialización segura de los motores y una respuesta I2C mínima para confirmar presencia.

## 8. Limitaciones y observaciones técnicas

- Si el maestro envía varios bytes en una sola transmisión I2C, solo se ejecutará el **último** byte recibido.
- La respuesta I2C siempre es `1`, por lo que no se reporta estado real del sistema, errores ni confirmación del comando ejecutado.
- No existen finales de carrera, validaciones de posición ni temporizadores de seguridad en el código analizado.
- No hay pruebas automatizadas en la carpeta `test`.
- No fue posible validar la compilación en este entorno porque el comando `pio` de PlatformIO no está instalado o no está disponible en la terminal actual.

## 9. Conclusión

El proyecto funciona como un **módulo esclavo de control por I2C** para un mecanismo con servo, base, antebrazo y pinza. Su comportamiento es directo y adecuado para recibir órdenes externas desde un maestro, pero actualmente depende de que ese maestro gestione la secuencia, duración y seguridad de los movimientos.

Como siguiente mejora, sería recomendable agregar retroalimentación de estado, validaciones de seguridad y una estructura de comandos más robusta si se planea usar en un entorno físico continuo.
