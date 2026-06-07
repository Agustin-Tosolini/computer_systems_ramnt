# TP5 Tools

Herramientas para el flujo notebook -> Raspberry Pi -> notebook:

- `gpio_jsonl_reader/`: lector C que se compila en la notebook, se copia a la Raspberry Pi y emite muestras como JSON Lines.
- `web_content/`: dashboard Docker que se conecta por SSH a la Raspberry Pi, ejecuta el lector remoto y grafica `value_a` y `value_b`.
- `KERNEL_DRIVER_RECOMMENDATIONS.md`: recomendaciones para definir el contrato del modulo/driver de kernel.

## Flujo recomendado

Desde la raiz del proyecto:

```bash
cd tools/gpio_jsonl_reader
make CC=aarch64-linux-gnu-gcc TARGET=build/gpio_jsonl_reader-aarch64
```

Copie el binario a la Raspberry Pi:

```bash
ssh <usuario>@<raspberry-host> 'mkdir -p ~/tp5'
scp build/gpio_jsonl_reader-aarch64 <usuario>@<raspberry-host>:~/tp5/gpio_jsonl_reader
ssh <usuario>@<raspberry-host> 'chmod +x ~/tp5/gpio_jsonl_reader'
```

Pruebe que el stream funcione antes de levantar la web:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100'
```

La salida esperada es una linea JSON por muestra:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":123,"value_b":456,"pin_a":17,"pin_b":27}
```

Si el TP requiere leer una direccion fisica:

```bash
ssh <usuario>@<raspberry-host> 'sudo ~/tp5/gpio_jsonl_reader --mem-address 0x10000000 --offset-a 0 --offset-b 4 --width 32 --interval-ms 100'
```

Use direcciones fisicas/MMIO o regiones reservadas documentadas. No use punteros virtuales del kernel como contrato con user space. Lo mas estable para el grupo es que el modulo exponga un char device y que el lector use `--device`.

## Levantar la visualizacion

Configure la web:

```bash
cd ../web_content
cp .env.example .env
```

Edite `.env` con los datos de SSH y el comando remoto:

```bash
SSH_HOST=raspberrypi.local
SSH_USER=pi
SSH_PRIVATE_KEY=/ssh/id_ed25519
REMOTE_COMMAND=/home/pi/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100
```

Levante el dashboard:

```bash
docker compose up --build
```

Abra:

```text
http://localhost:8080
```

## Estructura de datos esperada

El dashboard espera JSON Lines. Cada linea debe contener al menos:

```json
{"value_a":123,"value_b":456}
```

El lector agrega campos utiles como `timestamp_ms`, `seq`, `source`, `pin_a` y `pin_b`.

## Orden de trabajo sugerido para el grupo

1. Definir en el modulo de kernel como se exponen las dos muestras convertidas.
2. Preferir un char device que entregue dos valores por lectura o por linea de texto.
3. Compilar el lector en la notebook para la arquitectura de la Raspberry Pi.
4. Copiar el binario por SSH y probar el comando remoto manualmente.
5. Configurar `.env` en `tools/web_content`.
6. Levantar Docker Compose y revisar la grafica local.
