# TP2 - Estructura del proyecto

Este directorio contiene las distintas variantes desarrolladas para el Trabajo
Práctico 2 de Sistemas de Computación, junto con la documentación asociada al
proyecto.

El contenido principal puede leerse en el siguiente orden:

## 1. `documentation/`

Esta es la carpeta más importante desde el punto de vista de la entrega formal,
ya que contiene el informe del proyecto en sí.

- `informe_TP2.qd`: documento principal del trabajo práctico.
- `media/`: imágenes utilizadas como soporte visual dentro del informe.
- `output/`: archivos generados a partir de la compilación o exportación del
  informe, en donde se encuentra el PDF referente al mismo.


## 2. `without_docker/`

Corresponde a una primera versión del trabajo, sin contenedores Docker. Esta
variante sirve como aproximación inicial al problema y muestra la integración
entre Python, C y Assembly en un esquema más simple.

Archivos principales:

- `main.py`: punto de entrada en Python para la consulta y prueba del flujo.
- `client.py`: cliente que se comunica con el servidor de 32 bits.
- `server.py`: servidor que expone la biblioteca nativa al proceso Python.
- `bridge.c`: capa intermedia en C.
- `convert.asm`: rutina Assembly de conversión.
- `README.md`: instrucciones específicas de esta variante.
- `build/`: archivos compilados de esta implementación.

## 3. `docker_test/`

Representa la versión automatizada del flujo original. Aquí se incluye la
consulta a la API del Banco Mundial, la interoperabilidad con C y Assembly, la
exposición HTTP y la ejecución dentro de un entorno Docker reproducible.

Archivos principales:

- `api.py`: lógica principal del pipeline en Python.
- `interface_w_asm.c`: interfaz en C hacia la rutina nativa.
- `float_to_int.asm`: rutina Assembly para la conversión numérica.
- `Dockerfile`: definición del entorno de ejecución.
- `docker-compose.yml`: orquestación del contenedor.
- `run_pipeline.sh`: script que compila y ejecuta el flujo.
- `requirements.txt`: dependencias Python.
- `runtime/`: salida de compilación y logs generados en esta variante.
- `README.md`: guía de uso específica.

## 4. `c_program/`

Esta carpeta contiene la variante desarrollada completamente en C, pensada para
facilitar el análisis con GDB y observar con claridad el comportamiento del
stack, los frames y las variables locales durante la ejecución.

Archivos principales:

- `main.c`: implementación completa del flujo en C.
- `Makefile`: compilación del binario.
- `build/`: ejecutable generado.
- `README.md`: documentación de uso, pruebas y depuración con GDB.

Esta variante es especialmente útil para el análisis de convenciones de llamada
y del estado de la memoria durante la ejecución.
