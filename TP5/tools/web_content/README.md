# Web Content

Dashboard local dockerizado para graficar muestras JSON Lines generadas por `gpio_jsonl_reader`.

El modo principal es manual: usted ejecuta el lector remoto por SSH y redirige cada linea JSON al endpoint HTTP del dashboard. El modo automatico por SSH sigue disponible, pero es opcional.

## Requisitos

- Docker con Compose.
- Acceso SSH desde la notebook hacia la Raspberry Pi para ejecutar el lector manualmente.
- El binario `gpio_jsonl_reader` copiado y ejecutable en la Raspberry Pi.
- El modulo de kernel cargado y exponiendo los valores por dispositivo o por una direccion fisica valida.

## Configuracion

Desde esta carpeta:

```bash
cp .env.example .env
```

Edite `.env`:

```bash
WEB_PORT=8080
MAX_SAMPLES=10
REMOTE_COMMAND=
```

Con `REMOTE_COMMAND=` vacio, el contenedor no intenta conectarse por SSH ni ejecutar comandos remotos.

## Levantar la web

```bash
docker compose up --build
```

Abra:

```text
http://localhost:8080
```

## Enviar muestras manualmente

Con la web levantada, ejecute el lector por SSH desde la notebook y envie cada linea al dashboard:

```bash
ssh -o PubkeyAuthentication=no ramnt@ramnt.local '/home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500' \
  | while IFS= read -r line; do
      printf '%s\n' "$line" \
        | curl -fsS -X POST -H 'Content-Type: text/plain' --data-binary @- http://localhost:8080/api/samples >/dev/null
    done
```

`-o PubkeyAuthentication=no` fuerza autenticacion por password. Es util si la notebook tiene una clave SSH bloqueada y el agente muestra errores como `agent refused operation`.

Para probar solo algunas muestras:

```bash
ssh -o PubkeyAuthentication=no ramnt@ramnt.local '/home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500 --max-samples 10' \
  | while IFS= read -r line; do
      printf '%s\n' "$line" \
        | curl -fsS -X POST -H 'Content-Type: text/plain' --data-binary @- http://localhost:8080/api/samples >/dev/null
    done
```

Si `/dev/tp5_gpio` requiere permisos de administrador en la Raspberry Pi:

```bash
ssh -o PubkeyAuthentication=no ramnt@ramnt.local 'sudo /home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500' \
  | while IFS= read -r line; do
      printf '%s\n' "$line" \
        | curl -fsS -X POST -H 'Content-Type: text/plain' --data-binary @- http://localhost:8080/api/samples >/dev/null
    done
```

Para cortar la lectura, use `Ctrl+C` en la terminal donde esta corriendo el pipeline.

Para cambiar el puerto local:

```bash
WEB_PORT=8090 docker compose up --build
```

## Bajar la web

Si levanto el dashboard con el comando normal, bajelo desde esta carpeta con:

```bash
docker compose down
```

Para eliminar tambien contenedores huerfanos creados por cambios de configuracion:

```bash
docker compose down --remove-orphans
```

Si levanto una instancia alternativa con otro nombre de proyecto, por ejemplo:

```bash
WEB_PORT=8090 docker compose -p tp5-dashboard-8090 up --build
```

debe bajarla usando el mismo nombre de proyecto:

```bash
docker compose -p tp5-dashboard-8090 down --remove-orphans
```

Si Docker responde `permission denied` al detener el contenedor, reinicie el daemon y repita el `down`:

```bash
sudo systemctl restart docker
docker compose down --remove-orphans
```

Si aun asi sigue bloqueado, use el metodo de recuperacion manual:

```bash
docker update --restart=no <contenedor>
docker exec <contenedor> sh -c 'kill -TERM 1'
docker rm <contenedor>
```

Si `docker exec` no alcanza, obtenga el PID real del proceso en el host y terminelo con `sudo`:

```bash
PID=$(docker inspect -f '{{.State.Pid}}' <contenedor>)
sudo kill -9 "$PID"
docker rm <contenedor>
```

En instalaciones de Docker por Snap, puede ser necesario reiniciar el servicio Snap en lugar del servicio systemd tradicional:

```bash
sudo snap restart docker
```

## Error al detener un contenedor anterior

Si Docker muestra un error similar a:

```text
cannot stop container: <id>: permission denied
```

la imagen se construyo bien, pero el daemon de Docker no pudo detener un contenedor anterior. No es un error del codigo de la web.

Primero intente limpiar desde esta carpeta:

```bash
docker compose down --remove-orphans
docker compose up --build
```

Si el error continua, reinicie el servicio de Docker y vuelva a levantar:

```bash
sudo systemctl restart docker
docker compose up --build
```

Si Docker fue instalado como Snap, use:

```bash
sudo snap restart docker
docker compose up --build
```

Si no quiere reiniciar Docker en ese momento, puede levantar otra instancia temporal con otro nombre de proyecto y otro puerto:

```bash
WEB_PORT=8090 docker compose -p tp5-dashboard-8090 up --build
```

En ese caso abra:

```text
http://localhost:8090
```

## Formato recibido

El endpoint `/api/samples` recibe JSON Lines. Ejemplo:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":0,"value_b":1,"gpio_a_value":0,"gpio_b_value":1,"binary_code":"01","binary_value":1,"normalized_value":0.333333,"pin_a":17,"pin_b":27}
```

La web grafica `normalized_value` con eje fijo entre `0` y `1`, y muestra el codigo binario `00`, `01`, `10` o `11`. El grafico conserva solamente las ultimas 10 muestras. Si una linea no contiene un codigo valido, el backend la descarta y no agrega ningun punto.

El lector C esta preparado para leer el formato del driver:

```text
gpio_a=17 value=0
gpio_b=27 value=1
```

Cada ciclo abre `/dev/tp5_gpio`, lee, cierra y espera 500 ms antes de repetir.

## Modo automatico opcional por SSH

Si prefiere que el contenedor ejecute el lector remoto automaticamente, complete `REMOTE_COMMAND` y credenciales SSH en `.env`.

Con password:

```bash
SSH_HOST=ramnt.local
SSH_PORT=22
SSH_USER=ramnt
SSH_PRIVATE_KEY=
SSH_PASSPHRASE=
SSH_PASSWORD=<password>
REMOTE_COMMAND=/home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500
```

Con clave privada:

```bash
SSH_HOST=ramnt.local
SSH_PORT=22
SSH_USER=ramnt
SSH_PRIVATE_KEY=/ssh/id_ed25519
SSH_PASSPHRASE=<passphrase-si-corresponde>
SSH_PASSWORD=
REMOTE_COMMAND=/home/ramnt/Desktop/gpio_jsonl_reader-aarch64 --device /dev/tp5_gpio --interval-ms 500
```

El contenedor monta por defecto `${HOME}/.ssh` como `/ssh` en modo solo lectura. Por eso `SSH_PRIVATE_KEY` debe apuntar a una ruta dentro de `/ssh`.

## Variables de entorno

- `WEB_PORT`: puerto local publicado por Docker Compose. Default: `8080`.
- `SSH_HOST`: host o IP de la Raspberry Pi.
- `SSH_PORT`: puerto SSH. Default: `22`.
- `SSH_USER`: usuario remoto.
- `SSH_PRIVATE_KEY`: clave privada dentro del contenedor, por ejemplo `/ssh/id_ed25519`.
- `SSH_PASSPHRASE`: passphrase opcional para una clave privada cifrada.
- `SSH_PASSWORD`: password SSH opcional.
- `REMOTE_COMMAND`: opcional. Si queda vacio, la web usa modo manual y espera `POST /api/samples`.
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
