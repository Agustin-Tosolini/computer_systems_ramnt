# `txt_drivers_modules`

Este directorio reúne los scripts y reportes utilizados para relevar y comparar módulos del kernel y drivers detectados en distintas computadoras del grupo.

## Requisitos

- Sistema GNU/Linux.
- Acceso de lectura a `/proc/modules` y `/sys/bus`.
- `bash`, `find`, `awk`, `sed`, `sort`, `uname` y `hostname`.

## Scripts disponibles

### `drivers_modules`

Genera un archivo `.txt` con un reporte local de módulos cargados y drivers detectados.

Dar permisos de ejecución una única vez:

```bash
chmod +x drivers_modules
```

Luego ejecutar:

```bash
./drivers_modules <nombre_archivo>
```

Ejemplo:

```bash
./drivers_modules javier
```

El script genera un archivo de texto en la misma carpeta donde se encuentra el propio script. Si el nombre se pasa sin extensión, el script agrega `.txt` automáticamente.

Por ejemplo, al ejecutar:

```bash
./drivers_modules javier
```

se genera:

```text
javier.txt
```

El `.txt` incluye dos bloques principales:

1. `=== MODULOS CARGADOS ===`

Muestra la salida equivalente a `lsmod` con nombre del módulo, tamaño y contador de uso.

2. `=== DRIVERS DETECTADOS EN /sys/bus ===`

Muestra una tabla con bus, driver, módulo asociado y dispositivos enlazados.

### `compare_drivers_modules`

Compara todos los archivos `.txt` presentes en esta carpeta y muestra por consola:

- en verde, los módulos y drivers comunes a todos los reportes;
- en rojo, los módulos y drivers que no están presentes en todos los archivos.

Dar permisos de ejecución una única vez:

```bash
chmod +x compare_drivers_modules
```

Luego ejecutar:

```bash
./compare_drivers_modules
```

La comparación se realiza sobre criterios estables:

- módulos: se compara solo el nombre del módulo;
- drivers: se compara la combinación `bus/driver/módulo asociado`.

## Archivos de ejemplo y documentación adicional

- `agustin.txt`, `javier.txt`, `tomas.txt`: reportes generados en distintas máquinas.
- [README_compare_drivers_modules.md](./README_compare_drivers_modules.md): explicación específica del script comparador.

## Observaciones

- El apartado de drivers se construye a partir de `/sys/bus`, por lo que refleja lo que el kernel expone en tiempo de ejecución.
- Algunos drivers pueden aparecer como `builtin/no-visible`, lo que indica que están integrados en el kernel o que no exponen un vínculo visible hacia un módulo cargable.
- El contenido de cada reporte depende de la máquina y del estado del sistema al momento de la ejecución.
- El comparador no hace un `diff` textual bruto: resume coincidencias y diferencias semánticas entre reportes.
