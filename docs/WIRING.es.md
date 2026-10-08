# Conexiones - Llavero amigo (ESP32-C3 mini)

Diagrama: [wiring-diagram.es.png](wiring-diagram.es.png)

Pines definidos en [src/config.h](../src/config.h). Si cambias un cable, cambia el numero ahi.

## Pantalla ST7789 240x240 (SPI)

| Pantalla       | ESP32-C3 | Nota                                              |
|----------------|----------|---------------------------------------------------|
| VCC            | 3V3      | 3.3 V                                             |
| GND            | GND      |                                                   |
| SCL / SCK      | GPIO4    |                                                   |
| SDA / MOSI     | GPIO6    |                                                   |
| RES / RST      | GPIO3    |                                                   |
| DC             | GPIO5    |                                                   |
| BLK / BL       | GPIO10   | Luz de fondo. Tambien puede ir a 3V3 si sobran pines |
| CS             | (no usa) | Los modulos de 7 pines no traen CS. Si el tuyo lo trae, a GPIO7 y pon `PIN_TFT_CS 7` y `TFT_SPI_MODE 0` |

## Boton capacitivo (modulo TTP223)

El ESP32-C3 **no tiene sensor tactil capacitivo interno** (eso solo lo traen el ESP32, S2 y S3).
Se usa un modulo TTP223, que entrega una salida digital.

| TTP223 | ESP32-C3 |
|--------|----------|
| VCC    | 3V3      |
| GND    | GND      |
| I/O    | GPIO1    |

Cada toque pasa a la siguiente expresion: Feliz, Parpadeo, Enamorado, Bateria baja, Notificacion.

## Pila LiPo 3.7 V 1200 mAh, interruptor y medicion de voltaje

Interruptor de 2 posiciones (SPDT, usado como encendido/apagado) en el positivo de la pila:

| Desde | Hasta |
|-------|-------|
| Pila (+) | terminal comun del interruptor |
| terminal ON del interruptor | pin 5V/VIN del ESP32 **y** R1 del divisor |
| Pila (-) | GND comun |

El tercer terminal del interruptor queda sin usar. Al apagarlo se corta tambien el divisor, que si no gastaria un poco de bateria.

**El pin 5V solo sirve si tu placa tiene regulador de baja caida (ej. ME6211).** Con un AMS1117 una pila de 3.7 V no alcanza. Confirma tambien que la placa tenga diodo en el pin 5V, para que el USB no empuje 5 V hacia la pila. Usa una pila con circuito de proteccion.

**No conectes la pila directo al GPIO0.** Cargada llega a 4.2 V y el ESP32-C3 soporta 3.3 V como maximo. Usa un divisor con dos resistencias iguales:

| Desde | Hasta |
|-------|-------|
| interruptor (positivo conmutado) | R1 100 kOhm |
| R1 | nodo -> GPIO0 |
| nodo | R2 100 kOhm |
| R2 | GND |

Opcional: condensador de 100 nF entre el nodo y GND. Con la pila a 4.2 V el pin ve 2.1 V. El firmware multiplica por 2 y convierte a % con la curva de una LiPo (`PIN_BAT_ADC`, `BAT_DIV_RATIO` en [src/config.h](../src/config.h)). Si el pin queda al aire, el llavero lo detecta y usa un valor simulado desde la web.

## Pines que NO conviene usar
- GPIO8 (LED de la placa y pin de arranque), GPIO9 (boton BOOT), GPIO2 (pin de arranque).
- GPIO18 / GPIO19 (USB nativo, es por donde se programa).
- GPIO20 / GPIO21 (UART0).

## Ajustes si la pantalla se ve mal
- Colores invertidos: `TFT_INVERT` en [src/config.h](../src/config.h).
- Imagen corrida 80 px: `TFT_OFFSET_Y 80`.
- Pantalla en blanco: revisa `TFT_SPI_MODE` (3 sin CS, 0 con CS) y que BL tenga 3.3 V.

## Pendiente de definir
- Carga de la pila por USB: falta un modulo de carga con proteccion (TP4056 con proteccion o similar) y confirmar como alimenta tu placa C3 mini (pin 5V/VIN o 3V3). Depende del modelo exacto de placa.
- Pestana "Estado" (bateria y notificaciones del telefono) del diseno: no portada todavia.
