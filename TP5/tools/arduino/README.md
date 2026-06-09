# Arduino Waveform Former

Esta carpeta contiene el sketch `waveform_former.ino`, utilizado para generar una secuencia binaria repetitiva sobre dos salidas digitales de Arduino.

## Funcion del sketch

El programa configura los pines digitales `2` y `3` como salidas:

```cpp
const int gpioA = 2;
const int gpioB = 3;
```

Luego, en el `loop()`, conmuta ambos pines para recorrer las cuatro combinaciones posibles de dos bits:

```text
00 -> 01 -> 10 -> 11
```

Cada estado se mantiene durante 500 ms. Esto permite alimentar dos entradas GPIO de la Raspberry Pi con una senal conocida y periodica, de modo que el modulo de kernel pueda leer los cambios y el dashboard pueda graficar la secuencia observada.

## Secuencia generada

| Estado | Pin 2 (`gpioA`) | Pin 3 (`gpioB`) | Espera |
| --- | --- | --- | --- |
| `00` | LOW | LOW | 500 ms |
| `01` | LOW | HIGH | 500 ms |
| `10` | HIGH | LOW | 500 ms |
| `11` | HIGH | HIGH | 500 ms |

## Uso

1. Abrir `waveform_former.ino` en Arduino IDE o en una herramienta compatible.
2. Seleccionar la placa y el puerto serie correspondientes.
3. Cargar el sketch en el Arduino.
4. Conectar las salidas digitales al circuito de adaptacion de nivel y luego a los GPIO de entrada de la Raspberry Pi.

## Advertencia electrica

Las entradas GPIO de la Raspberry Pi 5 trabajan a 3.3 V y no son tolerantes a 5 V. Si se usa un Arduino UNO u otra placa con salidas de 5 V, no se deben conectar las salidas directamente a la Raspberry Pi. Es necesario usar divisores resistivos, un conversor de nivel logico o una etapa equivalente que limite la tension aplicada a los GPIO.

## Relacion con el resto del TP

El sketch cumple el rol de fuente de senales. La Raspberry Pi lee esas senales mediante el modulo de kernel `dual_gpio_input_module`, el programa `gpio_jsonl_reader` convierte cada lectura en JSON Lines, y la notebook recibe esas muestras por SSH para representarlas en el dashboard Docker.
