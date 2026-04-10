# TP2 - Flujo Python 64-bit + puente 32-bit (C/ASM)

Este proyecto implementa un flujo mixto de arquitecturas:

- Proceso principal en Python 64-bit ([main.py](main.py)).
- Servidor intermedio en Python 32-bit ([server.py](server.py)).
- Libreria nativa de 32 bits compilada desde C y Assembly ([bridge.c](bridge.c) + [convert.asm](convert.asm)).

El objetivo es que el proceso 64-bit pueda usar codigo nativo 32-bit sin cargarlo directamente, usando [msl-loadlib](https://pypi.org/project/msl-loadlib/) como puente.

## 1. Arquitectura y flujo de trabajo

1. [main.py](main.py) (64-bit) consulta la API del Banco Mundial y arma una lista de valores float.
2. [client.py](client.py) crea un cliente `Client64` y llama remotamente al metodo `bridge`.
3. [server.py](server.py) corre en 32-bit (`Server32`), carga `libbridge.so` y prepara arreglos C con `ctypes`.
4. La funcion `bridge(...)` de [bridge.c](bridge.c) recorre el arreglo y llama a `convert(...)` en [convert.asm](convert.asm).
5. `convert` convierte cada float a int y suma 1.
6. Los resultados vuelven al proceso 64-bit y se imprimen en consola.

## 2. Requisitos

### Herramientas del sistema (Linux)

Instalar compilador y soporte 32-bit:

```bash
sudo apt update
sudo apt install -y gcc g++ gcc-multilib g++-multilib libc6-dev-i386 nasm
```

### Python

- Python 64-bit para ejecutar [main.py](main.py).
- Python 32-bit para el servidor de [msl-loadlib](https://pypi.org/project/msl-loadlib/).
- Paquetes en el entorno que corresponda:

```bash
pip install requests msl-loadlib
```

Nota: dependiendo de tu setup, puede que necesites instalar `msl-loadlib` tanto en el Python 64-bit como en el Python 32-bit.

## 3. Compilacion de la libreria nativa (32 bits)

Desde la raiz del proyecto:

```bash
mkdir -p build
nasm -f elf32 convert.asm -o build/convert.o
gcc -m32 -fPIC -c bridge.c -o build/bridge.o
gcc -m32 -shared -o build/libbridge.so build/bridge.o build/convert.o
```

Como [server.py](server.py) intenta cargar `./libbridge.so`, deja una copia/symlink en la raiz:

```bash
cp build/libbridge.so ./libbridge.so
```

## 4. Ejecucion de la prueba

```bash
python main.py
```

Que deberias ver:

- Consulta a la API para el pais configurado en [main.py](main.py).
- Lista de valores Gini obtenidos.
- Resultado procesado por la libreria 32-bit (valores enteros).

## 5. Estructura de archivos importante

- [main.py](main.py): entrada principal, proceso 64-bit.
- [client.py](client.py): cliente RPC 64-bit hacia servidor 32-bit.
- [server.py](server.py): servidor 32-bit que carga `libbridge.so`.
- [bridge.c](bridge.c): puente C de procesamiento por lote.
- [convert.asm](convert.asm): rutina ASM x86 32-bit.
- [cdecl.h](cdecl.h): macros de calling convention.
- [build/](build/): objetos y libreria generada.

## 6. Problemas comunes y solucion

### Error al compilar con `-m32`

Faltan dependencias multilib.

- Solucion: instalar `gcc-multilib`, `g++-multilib`, `libc6-dev-i386`.

### `OSError: ./libbridge.so: cannot open shared object file`

La libreria no esta en la ruta esperada.

- Solucion: compilar y copiar `build/libbridge.so` a `./libbridge.so`.

### Falla al iniciar servidor 32-bit de msl-loadlib

No hay Python 32-bit disponible o no esta accesible desde el entorno actual.

- Solucion: instalar Python 32-bit y validar la configuracion de `msl-loadlib` para localizar el interprete de 32 bits.

## 7. Nota tecnica sobre la firma C

En [bridge.c](bridge.c), la funcion `bridge` esta declarada como `void`.
En [server.py](server.py), `restype` esta definido como `ctypes.c_int`.

Para evitar comportamiento indefinido, lo correcto es mantener firma consistente:

- O bien cambiar `restype` a `None` en [server.py](server.py).
- O bien cambiar la funcion C para que retorne un `int` (por ejemplo, codigo de estado).

Mientras no se unifique, el campo `status` que se imprime puede no ser confiable.

## 8. Resumen rapido

1. Instalar dependencias de sistema y Python.
2. Compilar [bridge.c](bridge.c) y [convert.asm](convert.asm) en 32 bits.
3. Copiar `build/libbridge.so` a la raiz.
4. Ejecutar [main.py](main.py) con Python 64-bit.
