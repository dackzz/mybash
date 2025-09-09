## To build:
```sh
make
```

## To use:
```sh
./mybash
```

Los archivos principales que implementamos son:

* `command.c`: define los TADs `scommand` y `pipeline`, que son la base para poder representar los comandos y armar nuestro propio bash.
* `execute.c`: es el corazón del programa, se encarga de ejecutar los comandos de los pipelines usando syscalls, haciendo redirecciones de entrada/salida y conectando las pipes entre sí.
* `parsing.c`: se ocupa de parsear la entrada, es decir, transformar el texto en estructuras abstractas de comandos que después se ejecutan más fácil.
* `builtin.c`: maneja los comandos internos del shell que ya están integrados en el sistema operativo.
* `mybash.c`: es el archivo que ejecuta todo, el REPL de nuestro shell.

Para organizarnos como grupo tuvimos varias complicaciones. Nos armamos tarde y al principio solo éramos dos, recién cuando logramos coordinar empezamos a dividirnos las tareas: cada uno se encargaba de un archivo `.c` distinto. Cuando alguno lo terminaba, nos juntábamos por Discord para revisarlo y ajustar detalles. Uno de los problemas más grandes fue que uno de los integrantes no tenía acceso al repositorio en Bitbucket, así que no podía ni pushear ni pullear; tuvimos que pasarnos los archivos por Discord para que otro los subiera. Más adelante se sumó una integrante extranjera, pero la comunicación fue buena y pudimos trabajar bien en equipo ya los tres juntos.

En relación a la IA, la utilizamos para corregir detalles, solucionar errores específicos y como guía para entender algunos procesos que eran difíciles de comprender. Principalmente nos ayudó a dejar comentarios claros en el código y a asimilar conceptos, facilitando así el trabajo para cada uno de nosotros.

Link Video: https://youtu.be/50une21GXeU

Lab1 MyBash Grupo50
- Facundo Mauvecin
- Patricio Rivadeneira