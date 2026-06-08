# TP5 Tools

Herramientas para el flujo notebook -> Raspberry Pi -> notebook:

- `gpio_jsonl_reader/`: lector C que se compila en la notebook, se copia a la Raspberry Pi y emite muestras como JSON Lines.
- `web_content/`: dashboard Docker que recibe JSON Lines por HTTP y grafica el codigo binario combinado de los GPIO.
- `KERNEL_DRIVER_RECOMMENDATIONS.md`: recomendaciones para definir el contrato del modulo/driver de kernel.

## Que hace el proyecto

El flujo completo queda dividido en tres partes:

1. El modulo de kernel en la Raspberry Pi expone `/dev/tp5_gpio`. Cada vez que alguien lee ese archivo, el driver toma una nueva medicion de los GPIO y devuelve algo como:

```text
gpio_a=17 value=0
gpio_b=27 value=1
```

2. El binario `gpio_jsonl_reader-aarch64`, ejecutado en la Raspberry Pi, abre `/dev/tp5_gpio`, lee esos valores, cierra el archivo, espera 500 ms y repite. Por cada lectura genera una linea JSON:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":0,"value_b":1,"gpio_a_value":0,"gpio_b_value":1,"binary_code":"01","binary_value":1,"normalized_value":0.333333,"pin_a":17,"pin_b":27}
```

3. La notebook levanta el dashboard web en Docker. En otra terminal, la notebook ejecuta el lector remoto por SSH y reenvia cada JSON al endpoint local `http://localhost:8080/api/samples`. La web recibe esas muestras y grafica las ultimas 10.

## Flujo recomendado

Desde la raiz del proyecto:

```bash
cd tools/gpio_jsonl_reader
make CC=aarch64-linux-gnu-gcc TARGET=build/gpio_jsonl_reader-aarch64
```

Copie el binario a la Raspberry Pi:

```bash
scp -o PubkeyAuthentication=no build/gpio_jsonl_reader-aarch64 ramnt@ramnt.local:/home/ramnt/Desktop/gpio_jsonl_reader-aarch64
ssh -o PubkeyAuthentication=no ramnt@ramnt.local 'chmod +x /home/ramnt/Desktop/gpio_jsonl_reader-aarch64'
```

Pruebe que el stream funcione antes de levantar la web:

```bash
ssh -o PubkeyAuthentication=no ramnt@ramnt.local '/home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500 --max-samples 10'
```

La salida esperada es una linea JSON por muestra:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":0,"value_b":1,"gpio_a_value":0,"gpio_b_value":1,"binary_code":"01","binary_value":1,"normalized_value":0.333333,"pin_a":17,"pin_b":27}
```

Si el TP requiere leer una direccion fisica:

```bash
ssh -o PubkeyAuthentication=no ramnt@ramnt.local 'sudo /home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --mem-address 0x10000000 --offset-a 0 --offset-b 4 --width 32 --interval-ms 500'
```

Use direcciones fisicas/MMIO o regiones reservadas documentadas. No use punteros virtuales del kernel como contrato con user space. Lo mas estable para el grupo es que el modulo exponga un char device y que el lector use `--device`.

## Levantar la visualizacion

Configure la web:

```bash
cd ../web_content
cp .env.example .env
```

Edite `.env` para modo manual:

```bash
WEB_PORT=8080
MAX_SAMPLES=10
REMOTE_COMMAND=
```

Levante el dashboard:

```bash
docker compose up --build
```

Abra:

```text
http://localhost:8080
```

En otra terminal, envie las muestras al dashboard:

```bash
ssh -o PubkeyAuthentication=no ramnt@ramnt.local '/home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500' \
  | while IFS= read -r line; do
      printf '%s\n' "$line" \
        | curl -fsS -X POST -H 'Content-Type: text/plain' --data-binary @- http://localhost:8080/api/samples >/dev/null
    done
```

`-o PubkeyAuthentication=no` fuerza el uso de password SSH y evita que el agente intente abrir una clave privada bloqueada.

## Estructura de datos esperada

El dashboard espera JSON Lines. Cada linea debe contener al menos:

```json
{"gpio_a_value":0,"gpio_b_value":1}
```

El lector agrega campos utiles como `timestamp_ms`, `seq`, `source`, `binary_code`, `binary_value`, `normalized_value`, `pin_a` y `pin_b`.

El grafico usa `normalized_value` con eje fijo entre `0` y `1`, y conserva solamente las ultimas 10 muestras.

## Orden de trabajo sugerido para el grupo

1. Definir en el modulo de kernel como se exponen las dos muestras convertidas.
2. Preferir un char device que entregue dos valores por lectura o por linea de texto.
3. Compilar el lector en la notebook para la arquitectura de la Raspberry Pi.
4. Copiar el binario por SSH y probar el comando remoto manualmente.
5. Configurar `.env` en `tools/web_content`.
6. Levantar Docker Compose y revisar la grafica local.
