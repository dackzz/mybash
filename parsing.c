#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "parsing.h"
#include "parser.h"
#include "command.h"

static char *limpiar_comillas(char *arg) { // elimina comillas simples o dobles que rodean el argumento
    size_t len = strlen(arg);
    if (len >= 2 && ((arg[0] == '"' && arg[len - 1] == '"') ||
                     (arg[0] == '\'' && arg[len - 1] == '\''))) {
        arg[len - 1] = '\0';
        memmove(arg, arg + 1, len);
    }
    return arg;
}

static scommand parse_scommand(Parser parser) { // analiza y construye un comando simple a partir del parser
    scommand result = scommand_new();
    bool saw_any_normal = false; // indica si se vio algún argumento normal
    bool continuar = true; // controla el ciclo de parseo

    while (continuar) {
        parser_skip_blanks(parser);

        arg_kind_t type;
        char *arg = parser_next_argument(parser, &type); // obtiene el siguiente argumento y su tipo

        if (arg != NULL) {
            if (type == ARG_NORMAL) { // si es un argumento normal, lo agrega al comando
                saw_any_normal = true;
                arg = limpiar_comillas(arg);
                scommand_push_back(result, arg);
            }

            if (type == ARG_INPUT) { // si es redirección de entrada, la establece en el comando
                scommand_set_redir_in(result, arg);
            }

            if (type == ARG_OUTPUT) { // si es redirección de salida, la establece en el comando
                scommand_set_redir_out(result, arg);
            }

            if (type != ARG_NORMAL && type != ARG_INPUT && type != ARG_OUTPUT) { // si es un tipo inválido, libera y retorna NULL
                continuar = false;
            }
        } else { // si no se obtuvo argumento, verifica el tipo para decidir si continuar o liberar y retornar NULL
            if (type == ARG_INPUT || type == ARG_OUTPUT) {
                scommand_destroy(result);
                return NULL;
            } else {
                continuar = false;
            }
        }
    }

    if (!saw_any_normal && scommand_is_empty(result)) { // si no se vio ningún argumento normal, libera y retorna NULL
        scommand_destroy(result);
        return NULL;
    }

    return result;
}


pipeline parse_pipeline(Parser parser)
{
    if (parser == NULL || parser_at_eof(parser)) { // verifica validez del parser
        fprintf(stderr, "error: parser inválido o EOF alcanzado\n");
        return NULL;
    }

    pipeline result = pipeline_new();
    bool error = false;

    scommand cmd = parse_scommand(parser); // analiza el primer comando simple
    if (cmd == NULL) {
        error = true;
    } else {
        pipeline_push_back(result, cmd);
    }

    while (!error) { // ciclo para analizar comandos simples separados por pipes
        parser_skip_blanks(parser);
        bool has_pipe = false;
        parser_op_pipe(parser, &has_pipe);

        if (!has_pipe) break; // si no hay pipe, termina el ciclo

        cmd = parse_scommand(parser); 
        if (cmd == NULL) {
            error = true;
        } else {
            pipeline_push_back(result, cmd);
        }
    }

    if (!error) { // analiza si el pipeline debe esperar o ejecutarse en background
        parser_skip_blanks(parser);
        bool is_background = false;
        parser_op_background(parser, &is_background);
        pipeline_set_wait(result, !is_background);
    }

    bool garbage = false;
    if (!parser_at_eof(parser)) { // si no está al final, verifica si hay basura
        parser_garbage(parser, &garbage);
    }

    if (error || pipeline_is_empty(result) || garbage) { // si hubo error, el pipeline está vacío o hay basura, libera y retorna NULL
        result = pipeline_destroy(result);
    }

    return result;
}