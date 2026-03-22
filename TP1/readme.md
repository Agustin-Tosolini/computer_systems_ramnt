# TP1 - Sistemas de Computación

Este directorio contiene el desarrollo del Trabajo Práctico 1 de la materia **Sistemas de Computación**. El contenido del TP está orientado al análisis de rendimiento desde dos perspectivas complementarias: por un lado, el uso de **benchmarks de hardware** para comparar componentes y asociar métricas con escenarios de uso reales; por otro, el estudio de **performance de programas** mediante la herramienta `gprof`, con el objetivo de observar el costo temporal de distintas funciones y relacionarlo con las características del hardware utilizado.

## Contenido del trabajo

El informe del TP desarrolla los siguientes ejes principales:

- Introducción al concepto de rendimiento en sistemas de computación.
- Benchmarking de hardware y relación entre pruebas sintéticas y actividades cotidianas.
- Comparativa de compilación del kernel de Linux como caso de análisis técnico.
- Evaluación de la aceleración entre distintos procesadores AMD Ryzen.
- Análisis de performance de código mediante `gprof`.
- Conclusiones técnicas a partir de los resultados obtenidos.

## Estructura del directorio

### `documentation/`

Contiene el material principal de documentación del trabajo.

- `Informe_TP1.qd`: archivo fuente del informe. Aquí se encuentra redactado el contenido técnico del TP, incluyendo portada, introducción, desarrollo, tablas comparativas, análisis de resultados y conclusión.
- `Informer_RAMnt_TP1.pdf`: versión exportada del informe en PDF.
- `output/`: carpeta utilizada por el proceso de generación del documento y sus archivos derivados.

### `assets/`

Incluye los recursos gráficos utilizados en el informe.

- `Sello_institucional_V2.png`: imagen institucional utilizada en la portada.
- `banner.png`: recurso gráfico adicional del TP.
- `tomas.png`, `javier.png`, `agustin.png`: capturas o imágenes asociadas a los resultados obtenidos por cada integrante, utilizadas en la sección de análisis con `gprof`.

### `scripts/`

Reúne los archivos de código y resultados experimentales usados para la parte práctica del trabajo.

- `test_gprof.c`: programa en C utilizado para realizar el análisis de profiling.
- `test_gprof_new.c`: variante o versión modificada del programa original para nuevas pruebas o ajustes.


### `scripts/results/`

Contiene los resultados individuales del análisis realizado por cada integrante.

- `analysis_JavierSinuka.txt`
- `analysis_TomasBrigido.txt`
- `analysis_TosoliniAgustin.txt`

Cada uno de estos archivos registra la salida o el análisis derivado de `gprof` para la máquina de cada integrante, permitiendo comparar cómo cambian los tiempos y costos de ejecución según el hardware disponible.

## Objetivo técnico del TP

El propósito de este trabajo no es únicamente presentar resultados, sino también **interpretarlos** desde una perspectiva de ingeniería. En particular, se busca:

- Comprender qué mide cada benchmark y en qué contexto resulta útil;
- Analizar cómo influyen la arquitectura del procesador, la cantidad de núcleos, la memoria y el almacenamiento sobre el rendimiento;
- Observar que la performance de un programa depende tanto del hardware como de la distribución del costo computacional entre sus funciones;
- Desarrollar criterio técnico para evaluar resultados de benchmarking y profiling.

## Observaciones

- El archivo principal a editar para modificar el contenido del informe es `documentation/Informe_TP1.qd`.
- Las imágenes del informe se referencian desde la carpeta `assets/`.
- Los resultados experimentales de la parte de profiling se encuentran en `scripts/results/`.
