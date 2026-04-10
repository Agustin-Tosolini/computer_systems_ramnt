# docker_test

Este proyecto ejecuta un pipeline automatizado con Docker Compose:

- consulta la API del World Bank desde Python
- compila el codigo C y ASM
- convierte valores `float` a `int`
- suma `1` a cada valor
- genera un grafico en `runtime/logs`

## Requisitos

Antes de ejecutar el proyecto, tenes que tener instalado:

- Docker
- Docker Compose

En instalaciones actuales, `docker compose` suele venir incluido con Docker.

## Instalar Docker

La forma recomendada es seguir la documentacion oficial de Docker segun tu sistema operativo:

- Linux: https://docs.docker.com/en/latest/installation/
- Docker Compose: https://docs.docker.com/compose/install/
- Docker Desktop: https://docs.docker.com/desktop/

### Linux

Si usas Linux, instala Docker Engine siguiendo la guia oficial para tu distribucion. Luego verifica:

```bash
docker --version
docker compose version
```

Si `docker compose version` no funciona, revisa la instalacion del plugin Compose en la documentacion oficial.

### Windows / macOS

Si usas Windows o macOS, instala Docker Desktop. Docker Desktop ya incluye Docker Compose.

Luego verifica:

```bash
docker --version
docker compose version
```

## Ejecutar el proyecto

Ubicate en esta carpeta:

```bash
cd /home/javier/Documents/University/Computer_Systems/TPs/TP2/docker_test
```

Luego ejecuta:

```bash
docker compose up --build
```

Si ya construiste la imagen antes y no hiciste cambios en el entorno de Docker, tambien podes ejecutar:

```bash
docker compose up
```

Si tu entorno usa el comando antiguo, podes probar:

```bash
docker-compose up --build
```

o sin reconstruir:

```bash
docker-compose up
```

## Que hace este comando

Al ejecutar `docker compose up --build`:

1. construye la imagen Docker
2. levanta el contenedor
3. compila `float_to_int.asm`
4. compila `interface_w_asm.c`
5. genera la libreria compartida
6. ejecuta `api.py`
7. consulta la API
8. convierte los valores usando C + ASM
9. genera el grafico final

## Archivos generados

Los archivos compilados quedan en:

```text
runtime/build
```

Los graficos generados quedan en:

```text
runtime/logs
```

## Detener el contenedor

Si lo levantaste en primer plano, podes detenerlo con:

```bash
Ctrl + C
```

Si queres bajar los servicios manualmente:

```bash
docker compose down
```
