# Robot Clasificador

Sistema de clasificación basado en Arduino y ESP32 para operaciones distribuidas.

## Estructura del Proyecto

```
robot-clasificador/
├── ESP32_maestro/          # Controlador maestro basado en ESP32
│   ├── src/
│   ├── include/
│   ├── lib/
│   ├── test/
│   └── platformio.ini
│
├── ESP32_esclavo/          # Nodo esclavo basado en Arduino/ESP32
│   ├── src/
│   ├── include/
│   ├── lib/
│   ├── test/
│   └── platformio.ini
│
├── docs/                   # Documentación del proyecto
│   ├── REPORTE_ESP32_maestro.md
│   └── REPORTE_Arduino_esclavo.md
│
└── README.md              # Este archivo
```

## Descripción dei Componentes

### ESP32_maestro
Controlador principal del sistema. Gestiona la comunicación con los nodos esclavos y la lógica de clasificación.

### ESP32_esclavo (Arduino_esclavo)
Nodo esclavo del sistema. Ejecuta tareas de captura de datos y actuación según instrucciones del maestro.

## Documentación

- [Reporte ESP32_maestro](docs/REPORTE_ESP32_maestro.md)
- [Reporte Arduino_esclavo](docs/REPORTE_Arduino_esclavo.md)

## Requisitos

- PlatformIO CLI o VS Code + PlatformIO Extension
- Git

## Instalación

1. Clona este repositorio:
   ```bash
   git clone https://github.com/Adriandelaluzm/robot-clasificador.git
   cd robot-clasificador
   ```

2. Navega a la carpeta del proyecto que deseas compilar:
   ```bash
   cd ESP32_maestro
   # o
   cd ESP32_esclavo
   ```

3. Compila y carga el código:
   ```bash
   platformio run --target upload
   ```

## Autor

Jadri

## Licencia

MIT
