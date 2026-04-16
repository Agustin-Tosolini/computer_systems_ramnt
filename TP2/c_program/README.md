# Flujo minimo para depuracion con GDB

Este ejemplo replica el flujo de `docker_test`, pero ahora todo se hace desde C:

1. `main.c` abre una conexion HTTPS a `api.worldbank.org`.
2. Descarga el JSON del indicador GINI para el rango pedido.
3. Filtra en C los registros del pais solicitado.
4. Convierte cada `float` a `int`.
5. Luego suma `1` a cada valor.
6. Finalmente imprime una tabla y un grafico ASCII por consola.

## Dependencias

- GCC
- OpenSSL (`libssl`, `libcrypto`)
- GDB

## Ejecucion

```bash
cd c_program
make
./build/gini_client Argentina
./build/gini_client "Brazil" 2012 2020
```

El binario compilado se guarda en la carpeta `build/`.

## Casos de prueba

### Compilacion y ejecucion

Caso 1: compilacion limpia y corrida base

```bash
cd c_program
make clean
make
./build/gini_client Argentina
```

Resultado esperado:

- Se recompila `build/gini_client`
- El programa muestra una tabla con datos de Argentina
- Al final imprime el grafico ASCII

Caso 2: consulta de otro pais

```bash
cd c_program
./build/gini_client Brazil 2012 2020
```

Resultado esperado:

- Se imprimen 9 puntos para Brazil
- La tabla muestra columnas `Anio`, `Valor API`, `Entero` y `Ajustado`

Caso 3: rango invalido

```bash
cd c_program
./build/gini_client Argentina 2020 2011
```

Resultado esperado:

- El programa termina con error
- Se informa que el anio inicial no puede ser mayor al anio final

Caso 4: pais sin coincidencias

```bash
cd c_program
./build/gini_client PaisInexistente 2011 2020
```

Resultado esperado:

- El programa termina con error
- Se informa que la API no devolvio datos para el pais solicitado

## Depuracion recomendada

```bash
cd c_program/build
gdb ./gini_client
```

Breakpoints utiles:

```gdb
break main
break build_world_bank_request_path
break open_tls_connection
break fetch_https_response
break extract_http_body
break parse_world_bank_json
break convert_points_to_int
break increment_points
run Argentina
```

Inspecciones utiles:

```gdb
info frame
info locals
bt
x/128bx http_response
print request_path
print response_size
print points[0]
print point_count
```

### Casos de prueba con GDB

Caso 1: verificar que el breakpoint en `main` funcione

```bash
cd c_program/build
gdb ./gini_client
```

Dentro de GDB:

```gdb
break main
run Argentina
bt
info locals
```

Resultado esperado:

- GDB se detiene al entrar en `main`
- `bt` muestra el frame actual
- `info locals` lista variables locales del frame

Caso 2: inspeccionar la respuesta HTTP antes del parseo

```bash
cd c_program/build
gdb ./gini_client
```

Dentro de GDB:

```gdb
break extract_http_body
run Argentina
print http_response
x/128bx http_response
continue
```

Resultado esperado:

- El breakpoint se detiene antes de separar headers y body
- `x/128bx http_response` deja ver bytes del buffer HTTP en memoria

Caso 3: revisar los puntos parseados

```bash
cd c_program/build
gdb ./gini_client
```

Dentro de GDB:

```gdb
break parse_world_bank_json
run Argentina
finish
print point_count
print points[0]
print points[1]
```

Resultado esperado:

- `finish` vuelve al llamador al terminar el parseo
- `count` refleja la cantidad de puntos encontrados
- `points[0]` y `points[1]` muestran anio y valor original cargados en memoria

Caso 4: validar la conversion y el ajuste `+1`

```bash
cd c_program/build
gdb ./gini_client
```

Dentro de GDB:

```gdb
break convert_points_to_int
break increment_points
run Argentina
continue
print points[0]
print points[1]
continue
print points[0]
print points[1]
```

Resultado esperado:

- Tras `convert_points_to_int`, `converted_value` deja de ser `0`
- Tras `increment_points`, `adjusted_value` queda en `converted_value + 1`

## Verificacion rapida

```bash
cd c_program
make clean
make
./build/gini_client Argentina
```
