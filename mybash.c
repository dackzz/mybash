#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "command.h"
#include "execute.h"
#include "parser.h"
#include "parsing.h"
#include "builtin.h"

static void show_prompt(void)
{
    printf("mybash> ");
    fflush(stdout);
}

int main(int argc, char *argv[])
{
    pipeline pipe;
    Parser input;
    bool quit = false;

    input = parser_new(stdin);
    while (!quit)
    {
        show_prompt();
        pipe = parse_pipeline(input);

        quit = parser_at_eof(input);
        if (pipe != NULL)
        {
            bool alone = builtin_alone(pipe); // vemos si el pipeline contiene un unico comando interno
            if (alone) {
                scommand front = pipeline_front(pipe);
                builtin_run(front); // ejecutamos el comando interno
            } else {
                execute_pipeline(pipe); // si no es un comando interno, ejecutamos el pipeline normalmente
            }
            pipe = pipeline_destroy(pipe);
        } else {
            if (quit) { // si es EOF, salimos limpiamente (Ctrl+D)
                putchar('\n');
                quit = true;
            } else {
            fprintf(stderr, "Error: comando inválido o error de sintaxis.\n");
            }
        }
    }
    parser_destroy(input);
    input = NULL;
    return EXIT_SUCCESS;
}
