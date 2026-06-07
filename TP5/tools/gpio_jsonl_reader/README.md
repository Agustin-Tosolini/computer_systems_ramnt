# GPIO JSONL Reader

Lector en C pensado para ejecutarse en la Raspberry Pi y escribir las muestras por `stdout` como JSON Lines. La notebook puede ejecutarlo por SSH y consumir una linea por muestra.

La opcion recomendada es que el modulo de kernel exponga los valores mediante un archivo de dispositivo, por ejemplo `/dev/tp5_gpio`, `/proc/tp5_gpio` o un archivo `sysfs`. Leer direcciones arbitrarias de memoria de kernel desde user space no es portable ni seguro. Si el TP requiere una direccion, use una direccion fisica/MMIO o una region reservada accesible por `/dev/mem`, no un puntero virtual del kernel.

## Formato de salida

Cada muestra se imprime como una linea JSON independiente:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":123,"value_b":456,"pin_a":17,"pin_b":27}
```

Campos principales:

- `timestamp_ms`: tiempo UNIX en milisegundos tomado en la Raspberry Pi.
- `seq`: contador local de muestra.
- `source`: `device` o `mem`.
- `value_a` y `value_b`: valores convertidos.
- `pin_a` y `pin_b`: etiquetas opcionales de GPIO, o `null`.

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

## Ejecucion recomendada con dispositivo del modulo

El modulo puede exponer una linea de texto con dos numeros, por ejemplo:

```text
123 456
```

En ese caso:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100'
```

Si el dispositivo entrega muestras binarias como dos `uint32_t` consecutivos:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --device-format binary-u32 --pin-a 17 --pin-b 27'
```

## Ejecucion con direccion fisica

Use este modo solo si el modulo o el hardware documenta una direccion fisica/MMIO valida. El programa mapea `/dev/mem`, lee dos registros y escribe JSONL.

```bash
ssh <usuario>@<raspberry-host> 'sudo ~/tp5/gpio_jsonl_reader --mem-address 0x10000000 --offset-a 0 --offset-b 4 --width 32 --interval-ms 100 --pin-a 17 --pin-b 27'
```

Opciones importantes:

- `--mem-address`: direccion fisica base.
- `--offset-a` y `--offset-b`: offsets en bytes para `value_a` y `value_b`.
- `--width`: ancho de lectura de cada registro: `8`, `16` o `32`.
- `--interval-ms`: intervalo entre muestras.
- `--max-samples`: cantidad maxima de muestras, `0` significa infinito.

Para evitar usar `sudo` en el flujo final, es preferible que el driver cree un char device con permisos de grupo adecuados y usar `--device`.
