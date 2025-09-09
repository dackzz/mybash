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
* `mybash.c`: archivo que ejecuta todo, el REPL de nuestro shell.
