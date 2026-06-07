# GPIO JSONL Reader

Lector en C pensado para ejecutarse en la Raspberry Pi y escribir las muestras por `stdout` como JSON Lines. La notebook puede ejecutarlo por SSH y consumir una linea por muestra.

La opcion recomendada es que el modulo de kernel exponga los valores mediante un archivo de dispositivo, por ejemplo `/dev/tp5_gpio`, `/proc/tp5_gpio` o un archivo `sysfs`. Leer direcciones arbitrarias de memoria de kernel desde user space no es portable ni seguro. Si el TP requiere una direccion, use una direccion fisica/MMIO o una region reservada accesible por `/dev/mem`, no un puntero virtual del kernel.

## Formato de salida

Cada muestra se imprime como una linea JSON independiente:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":0,"value_b":1,"gpio_a_value":0,"gpio_b_value":1,"binary_code":"01","binary_value":1,"normalized_value":0.333333,"pin_a":17,"pin_b":27}
```

Campos principales:

- `timestamp_ms`: tiempo UNIX en milisegundos tomado en la Raspberry Pi.
- `seq`: contador local de muestra.
- `source`: `device` o `mem`.
- `gpio_a_value` y `gpio_b_value`: valores leidos para cada GPIO, normalizados a `0` o `1`.
- `binary_code`: codigo combinado, por ejemplo `00`, `01`, `10` o `11`.
- `binary_value`: valor decimal del codigo, de `0` a `3`.
- `normalized_value`: valor para graficar con eje fijo entre `0` y `1`.
- `pin_a` y `pin_b`: GPIO detectados desde el device o etiquetas opcionales pasadas por parametro.

## Compilacion en la notebook

Desde la raiz del proyecto:

```bash
cd tools/gpio_jsonl_reader
make
```

Eso genera:

```bash
build/gpio_jsonl_reader
```

Si la notebook no es ARM64 y la Raspberry Pi usa Raspberry Pi OS 64-bit, compile cruzado:

```bash
sudo apt install gcc-aarch64-linux-gnu make
make clean
make CC=aarch64-linux-gnu-gcc TARGET=build/gpio_jsonl_reader-aarch64
```

Si la Raspberry Pi usa un sistema de 32 bits, el compilador suele ser:

```bash
sudo apt install gcc-arm-linux-gnueabihf make
make clean
make CC=arm-linux-gnueabihf-gcc TARGET=build/gpio_jsonl_reader-armhf
```

Puede verificar el tipo de binario con:

```bash
file build/gpio_jsonl_reader-aarch64
```

## Enviar el binario por SSH

Ejemplo generico, desde `tools/gpio_jsonl_reader`:

```bash
ssh <usuario>@<raspberry-host> 'mkdir -p ~/tp5'
scp build/gpio_jsonl_reader-aarch64 <usuario>@<raspberry-host>:~/tp5/gpio_jsonl_reader
ssh <usuario>@<raspberry-host> 'chmod +x ~/tp5/gpio_jsonl_reader'
```

Reemplace `<usuario>` y `<raspberry-host>` por los datos reales de la Raspberry Pi. Si compilo con otro `TARGET`, ajuste el nombre del archivo local.

## Ejecutar el binario en la Raspberry Pi

Una vez copiado el binario, se ejecuta por SSH como cualquier comando remoto.

Ejemplo normal:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --interval-ms 500'
```

Ese comando queda ejecutandose en loop. Cada 500 ms el programa:

1. abre `/dev/tp5_gpio`;
2. lee el contenido completo;
3. cierra `/dev/tp5_gpio`;
4. convierte la muestra a JSON Lines;
5. escribe la linea JSON por `stdout`.

Como `stdout` viaja por la conexion SSH, la notebook recibe una linea JSON por muestra.

Para cortar la ejecucion manualmente:

```text
Ctrl+C
```

Para probar una cantidad limitada de muestras:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --interval-ms 500 --max-samples 10'
```

Si `/dev/tp5_gpio` requiere permisos de administrador:

```bash
ssh <usuario>@<raspberry-host> 'sudo ~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --interval-ms 500'
```

El dashboard Docker usa este mismo comando mediante la variable `REMOTE_COMMAND`:

```bash
REMOTE_COMMAND=/home/pi/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --interval-ms 500
```

Si usa otro usuario o copio el binario en otra ruta, ajuste `/home/pi/tp5/gpio_jsonl_reader` por la ruta real.

## Ejecucion recomendada con dispositivo del modulo

El modulo usado en el TP expone dos lineas al leer `/dev/tp5_gpio`, por ejemplo:

```text
gpio_a=17 value=0
gpio_b=27 value=1
```

El lector abre el dispositivo, lee el contenido completo, cierra el dispositivo, espera 500 ms y repite. Esto es importante porque el driver genera una nueva medicion cada vez que se accede al archivo.

En ese caso:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --interval-ms 500'
```

Si el driver no informa los numeros de GPIO en el texto, puede etiquetarlos manualmente:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 500'
```

Si el dispositivo entrega muestras binarias como dos `uint32_t` consecutivos:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --device-format binary-u32 --pin-a 17 --pin-b 27'
```

## Ejecucion con direccion fisica

Use este modo solo si el modulo o el hardware documenta una direccion fisica/MMIO valida. El programa mapea `/dev/mem`, lee dos registros y escribe JSONL.

```bash
ssh <usuario>@<raspberry-host> 'sudo ~/tp5/gpio_jsonl_reader --mem-address 0x10000000 --offset-a 0 --offset-b 4 --width 32 --interval-ms 500 --pin-a 17 --pin-b 27'
```

Opciones importantes:

- `--mem-address`: direccion fisica base.
- `--offset-a` y `--offset-b`: offsets en bytes para `value_a` y `value_b`.
- `--width`: ancho de lectura de cada registro: `8`, `16` o `32`.
- `--interval-ms`: intervalo entre muestras.
- `--max-samples`: cantidad maxima de muestras, `0` significa infinito.

Para evitar usar `sudo` en el flujo final, es preferible que el driver cree un char device con permisos de grupo adecuados y usar `--device`.
