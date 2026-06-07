# Web Content

Dashboard local dockerizado para conectarse por SSH a la Raspberry Pi, ejecutar el lector C remoto y graficar las muestras recibidas como JSON Lines.

## Requisitos

- Docker con Compose.
- Acceso SSH desde la notebook hacia la Raspberry Pi.
- El binario `gpio_jsonl_reader` copiado y ejecutable en la Raspberry Pi.
- El modulo de kernel cargado y exponiendo los valores por dispositivo o por una direccion fisica valida.

## Configuracion

Desde esta carpeta:

```bash
cp .env.example .env
```

Edite `.env`:

```bash
SSH_HOST=raspberrypi.local
SSH_PORT=22
SSH_USER=pi
SSH_PRIVATE_KEY=/ssh/id_ed25519
SSH_PASSPHRASE=
REMOTE_COMMAND=/home/pi/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100
```

El contenedor monta por defecto `${HOME}/.ssh` como `/ssh` en modo solo lectura. Por eso `SSH_PRIVATE_KEY` debe apuntar a una ruta dentro de `/ssh`.

Si las claves estan en otra carpeta:

```bash
SSH_KEY_DIR=/ruta/a/claves docker compose up --build
```

Tambien puede usar `SSH_PASSWORD`, aunque para el trabajo grupal es mas simple y repetible usar clave SSH:

```bash
ssh-copy-id <usuario>@<raspberry-host>
```

Si la clave privada esta cifrada, complete `SSH_PASSPHRASE` en `.env` o use una clave de despliegue especifica para el TP.

## Levantar la web

```bash
docker compose up --build
```

Abra:

```text
http://localhost:8080
```

Para cambiar el puerto local:

```bash
WEB_PORT=8090 docker compose up --build
```

## Comando remoto esperado

`REMOTE_COMMAND` debe imprimir JSON Lines por `stdout`. Ejemplo:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":123,"value_b":456,"pin_a":17,"pin_b":27}
```

La web grafica `value_a` y `value_b`. Tambien acepta algunos alias (`channel_a`, `gpio_a`, `signal_a`, `value0`, etc.), pero el formato recomendado es el del lector C.

## Ejemplo con char device

```bash
REMOTE_COMMAND=/home/pi/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100
```

## Ejemplo con /dev/mem

Si se usa `/dev/mem`, normalmente se requiere `sudo`:

```bash
REMOTE_COMMAND=sudo /home/pi/tp5/gpio_jsonl_reader --mem-address 0x10000000 --offset-a 0 --offset-b 4 --width 32 --interval-ms 100
```

Para evitar pedir password dentro del contenedor, configure una regla `sudoers` especifica para ese binario o, preferentemente, ajuste el driver para exponer un dispositivo con permisos de grupo.

## Variables de entorno

- `WEB_PORT`: puerto local publicado por Docker Compose. Default: `8080`.
- `SSH_HOST`: host o IP de la Raspberry Pi.
- `SSH_PORT`: puerto SSH. Default: `22`.
- `SSH_USER`: usuario remoto.
- `SSH_PRIVATE_KEY`: clave privada dentro del contenedor, por ejemplo `/ssh/id_ed25519`.
- `SSH_PASSPHRASE`: passphrase opcional para una clave privada cifrada.
- `SSH_PASSWORD`: password SSH opcional.
- `REMOTE_COMMAND`: comando que se ejecuta en la Raspberry Pi.
- `MAX_SAMPLES`: muestras retenidas por el backend.
- `RECONNECT_MS`: espera entre reconexiones SSH.
- `SSH_READY_TIMEOUT_MS`: timeout inicial de SSH.

## Ejecucion local sin Docker

Para desarrollo:

```bash
npm install
npm start
```

Luego abra `http://localhost:8080`.
