# Trabajo Práctico 4

Este directorio corresponde al **Trabajo Práctico 4** de la materia **Sistemas de Computación**. El objetivo general del trabajo es introducir el uso de **módulos del kernel en Linux**, su compilación, carga y descarga dinámica, la inspección de su estado en ejecución y el análisis de aspectos de integridad y seguridad vinculados con `modinfo`, `lsmod`, `/proc/modules` y `Secure Boot`.

Los lineamientos, el desarrollo realizado y las respuestas teóricas asociadas se encuentran documentados en [documentation/Informe_TP4.qd](./documentation/Informe_TP4.qd).

## Estructura del proyecto

- [documentation](./documentation): contiene el material de documentación del trabajo.
- [documentation/output](./documentation/output): salida renderizada del informe en PDF.
- [documentation/txt_drivers_modules](./documentation/txt_drivers_modules): scripts y archivos auxiliares para relevar drivers y módulos cargados en una PC, exportarlos a `.txt` y comparar reportes entre distintos integrantes.
- [files_project](./files_project): reúne los archivos prácticos utilizados durante el desarrollo del trabajo.


## Alcance del trabajo

En este TP se trabajó principalmente con:

- compilación de un módulo del kernel propio;
- carga y descarga dinámica con `insmod` y `rmmod`;
- verificación de estado mediante `dmesg`, `lsmod` y `/proc/modules`;
- comparación entre un módulo propio y un módulo del sistema usando `modinfo`;
- comparación entre reportes de distintos equipos para identificar módulos y drivers comunes o diferenciales;
- discusión conceptual sobre diferencias entre programas y módulos, espacio de usuario y espacio del kernel, drivers y consideraciones de seguridad relacionadas con `Secure Boot`.

Este `README` busca únicamente orientar la navegación del repositorio. Para el desarrollo completo y el análisis académico, la referencia principal es el informe.
