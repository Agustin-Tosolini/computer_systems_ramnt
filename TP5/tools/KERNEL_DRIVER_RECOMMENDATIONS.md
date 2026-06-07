# Recomendaciones para el modulo/driver de kernel

Este documento resume el contrato recomendado entre el modulo de kernel que corre en la Raspberry Pi y las herramientas de user space incluidas en `tools/`.

El objetivo es que el driver exponga dos valores convertidos desde GPIO y que el lector `gpio_jsonl_reader` pueda leerlos en la Raspberry Pi para enviarlos como JSON Lines hacia la notebook.

## API recomendada

La opcion recomendada es que el modulo exponga un char device:

```text
/dev/tp5_gpio
```

El programa de user space puede leer ese dispositivo con:

```bash
~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100
```

Esto evita depender de direcciones internas del kernel y deja una interfaz clara entre kernel space y user space.

## Formato de datos

El formato mas simple para el driver es texto plano, con una muestra por linea:

```text
123 456
124 459
125 463
```

Cada linea debe contener:

- primer valor convertido: `value_a`;
- segundo valor convertido: `value_b`;
- salto de linea final.

El lector C convierte esa salida a JSON Lines:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":123,"value_b":456,"pin_a":17,"pin_b":27}
```

Si se prefiere un formato binario, el driver puede entregar exactamente dos `uint32_t` consecutivos por muestra:

```c
struct tp5_sample {
    uint32_t value_a;
    uint32_t value_b;
};
```

En ese caso, el lector debe ejecutarse con:

```bash
~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --device-format binary-u32 --pin-a 17 --pin-b 27
```

Para el TP, el formato texto suele ser mejor al principio porque se puede depurar con:

```bash
cat /dev/tp5_gpio
```

## Evitar pasar direcciones virtuales del kernel

No conviene que el modulo le entregue al programa una direccion virtual del kernel para que user space lea directamente.

Problemas de ese enfoque:

- no es una API estable;
- puede romperse entre versiones de kernel;
- requiere permisos elevados;
- mezcla detalles internos del modulo con user space;
- complica la seguridad y la depuracion.

Si la consigna pide almacenar los datos en memoria, el modulo puede mantener esa memoria internamente, pero user space deberia leerla mediante `/dev/tp5_gpio`.

El lector incluye un modo `--mem-address` como alternativa, pero deberia usarse solo si el TP exige leer una direccion fisica/MMIO valida mediante `/dev/mem`.

## No generar JSON en kernel space

El modulo no deberia generar JSON. Es preferible que el driver entregue datos simples:

```text
123 456
```

Luego `gpio_jsonl_reader` se encarga de convertirlos a JSON Lines.

Ventajas:

- el modulo queda mas chico;
- se evita logica innecesaria dentro del kernel;
- se reduce el riesgo de errores de formato;
- user space puede cambiar el formato de salida sin recompilar el modulo.

## Coherencia de muestras

Cada muestra debe representar un estado coherente:

```text
value_a value_b
```

`value_a` y `value_b` deberian corresponder al mismo instante logico de conversion o al mismo ciclo de muestreo.

Si hay concurrencia entre interrupciones, timers, `read()` y actualizaciones internas, proteger los datos con el mecanismo adecuado:

- `spinlock` si se accede desde contexto de interrupcion;
- `mutex` si se trabaja solo en contexto de proceso;
- variables atomicas si el caso es simple;
- ring buffer si se acumulan multiples muestras.

## Evitar busy waiting

El modulo no deberia hacer loops activos esperando cambios en GPIO.

Opciones mas adecuadas:

- interrupciones GPIO si las entradas son eventos digitales;
- timer o delayed work si se requiere muestreo periodico;
- `wait_queue` para que `read()` bloquee hasta que haya una nueva muestra;
- ring buffer para no perder muestras si user space lee mas lento.

Un buen comportamiento para `/dev/tp5_gpio` es:

- si hay muestra disponible, `read()` la devuelve;
- si no hay muestra disponible, `read()` puede bloquear hasta que exista una nueva;
- si el dispositivo se abre en modo no bloqueante, devolver `-EAGAIN` cuando no haya datos.

## Permisos del dispositivo

Conviene evitar depender de `sudo` para ejecutar el lector.

El driver o las reglas `udev` pueden dejar el dispositivo con permisos de grupo, por ejemplo:

```text
crw-rw---- root gpio /dev/tp5_gpio
```

Luego el usuario remoto de la Raspberry Pi se agrega al grupo correspondiente:

```bash
sudo usermod -aG gpio <usuario>
```

Esto simplifica el flujo SSH y el dashboard Docker, porque `REMOTE_COMMAND` no necesita ejecutar `sudo`.

## Parametros configurables

Es recomendable que el modulo permita configurar los GPIO y el intervalo de muestreo.

Ejemplo con parametros del modulo:

```bash
sudo insmod tp5_gpio.ko gpio_a=17 gpio_b=27 interval_ms=100
```

Parametros utiles:

- `gpio_a`: primer GPIO de entrada;
- `gpio_b`: segundo GPIO de entrada;
- `interval_ms`: intervalo de muestreo si se usa timer;
- `buffer_size`: cantidad de muestras retenidas;
- `debug`: activar logs de diagnostico.

## Consideraciones para Raspberry Pi 5

En Raspberry Pi 5, evitar hardcodear direcciones de registros salvo que sea estrictamente necesario y este documentado.

Preferir APIs del kernel:

- GPIO descriptors;
- Device Tree;
- `gpiod_get`;
- interrupciones GPIO administradas por el kernel;
- char device como interfaz de user space.

Si se usa MMIO directo, documentar:

- modelo exacto de Raspberry Pi;
- version de kernel;
- direccion base usada;
- offsets;
- fuente de la informacion;
- permisos necesarios para leer esa region.

## Contrato final esperado

El contrato ideal para este TP es:

```bash
cat /dev/tp5_gpio
```

Salida:

```text
123 456
124 459
125 463
```

Ejecucion desde la notebook por SSH:

```bash
ssh <usuario>@<raspberry-host> '~/tp5/gpio_jsonl_reader --device /dev/tp5_gpio --pin-a 17 --pin-b 27 --interval-ms 100'
```

Salida recibida por la notebook:

```json
{"timestamp_ms":1710000000000,"seq":1,"source":"device","value_a":123,"value_b":456,"pin_a":17,"pin_b":27}
{"timestamp_ms":1710000000100,"seq":2,"source":"device","value_a":124,"value_b":459,"pin_a":17,"pin_b":27}
```

Ese stream es el que consume el dashboard de `tools/web_content`.
