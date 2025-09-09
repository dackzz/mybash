#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <glib.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include "tests/syscall_mock.h"
#include "execute.h"
#include "builtin.h"
#include "parsing.h"
#include "command.h"

static char **scommand_to_argv(scommand self)
{
    assert(self != NULL);
    size_t len = scommand_length(self);
    char **argv = malloc((len + 1) * sizeof(char *));
    if (!argv){
        return NULL;
    }
    // backup para poder restaurar el scommand 
    char **backup = malloc(len * sizeof(char *));
    if (!backup) {
        free(argv);
        return NULL;
    }

    for (size_t i = 0; i < len; ++i) {
        char *arg = scommand_front(self);           // obtener puntero actual
        argv[i] = arg ? strdup(arg) : strdup("");  // argv recibe copia segura
        backup[i] = arg ? strdup(arg) : strdup(""); // backup copia segura para restaurar
        scommand_pop_front(self);                   // pop libera la cadena original
    }
    argv[len] = NULL;

    for (size_t i = 0; i < len; ++i) {
        scommand_push_back(self, backup[i]); // restaurar el scommand
    }
    free(backup); // liberamos solo el array de punteros (no las strings)

    return argv;
}


void execute_pipeline(pipeline apipe) 
{
    assert(apipe != NULL);

    if (pipeline_is_empty(apipe)) {
        return;
    }
    if (builtin_alone(apipe)) { // si es un comando interno, lo corre
        builtin_run(pipeline_front(apipe)); 
        return;
    }

    if (!pipeline_get_wait(apipe)) { // evita zombies ignorando SIGCHLD si no se espera a los hijos
        signal(SIGCHLD, SIG_IGN);
    }

    int total = pipeline_length(apipe);
    pid_t *pids = calloc(total, sizeof(pid_t)); // array para guardar los pids de los hijos
    if (pids == NULL)
    {
        perror("calloc");
        return;
    }

    int prev_fd = -1;
    bool error = false;

    for (int i = 0; i < total; ++i) {
        if (error) {
            return;
        } // descartar comandos restantes

        scommand scom = pipeline_front(apipe); // obtener el siguiente comando
        if (builtin_is_internal(scom))
        {
            error = true;
            break;
        }
        bool keep_going = (i < total - 1); // si hay más comandos, crear pipe
        int pipefd[2] = {-1, -1}; // inicializar pipe para no tener basura

        if (keep_going) { 
            if (pipe(pipefd) < 0) { // crear pipe
                perror("pipe");
                if (prev_fd != -1)
                    close(prev_fd); // cerrar fd previo si existe
                error = true;
            }
        }

        if (!error) {
            pid_t pid = fork(); // crear proceso hijo
            if (pid < 0) {
                perror("fork");
                if (prev_fd != -1) // cerrar fd previo si existe
                    close(prev_fd);
                if (keep_going) { // cerrar pipe si se creó
                    close(pipefd[0]);
                    close(pipefd[1]);
                }
                error = true;
            } else if (pid == 0) { // hijo
                // stdin desde prev_fd si existe 
                if (prev_fd != -1) {
                    int ret = dup2(prev_fd, STDIN_FILENO);
                    if (ret < 0) {
                        fprintf(stderr, "Error al redirigir la entrada desde pipe anterior: %s\n", strerror(errno));
                        exit(1);
                    }
                }

                // stdout hacia pipe siguiente si existe
                if (keep_going) {
                    int ret = dup2(pipefd[1], STDOUT_FILENO);
                    if (ret < 0) {
                        fprintf(stderr, "Error al redirigir la salida hacia pipe siguiente: %s\n", strerror(errno));
                        exit(1);
                    }
                }

                /* redirecciones de archivo */
                char *redir_in = scommand_get_redir_in(scom);
                if (redir_in != NULL) {
                    int fd = open(redir_in, O_RDONLY, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "Error al abrir archivo de entrada '%s': %s\n", redir_in, strerror(errno));
                        exit(1);
                    }
                    int ret = dup2(fd, STDIN_FILENO);
                    if (ret < 0) {
                        close(fd);
                        fprintf(stderr, "Error al redirigir la entrada desde '%s': %s\n", redir_in, strerror(errno));
                        exit(1);
                    }
                    close(fd);
                }

                char *redir_out = scommand_get_redir_out(scom);
                if (redir_out != NULL) {
                    int fd = open(redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0666);
                    if (fd < 0) {
                        fprintf(stderr, "Error al abrir archivo de salida '%s': %s\n", redir_out, strerror(errno));
                        exit(1);
                    }
                    int ret = dup2(fd, STDOUT_FILENO);
                    if (ret < 0) {
                        close(fd);
                        fprintf(stderr, "Error al redirigir la salida hacia '%s': %s\n", redir_out, strerror(errno));
                        exit(1);
                    }
                    close(fd);
                }
                if (prev_fd != -1) 
                    close(prev_fd);
                if (keep_going) {
                    close(pipefd[0]);
                    close(pipefd[1]);
                }

                char **argv = scommand_to_argv(scom); 
                if (!argv) {
                    perror("scommand_to_argv");
                    exit(1);
                }

                execvp(argv[0], argv);
                fprintf(stderr, "%s: comando no encontrado.\n", argv[0]);
                for (size_t j = 0; argv[j] != NULL; ++j)
                    free(argv[j]);
                free(argv);
                exit(1);
            } else { /* padre */
                pids[i] = pid; // guardar pid del hijo
                if (prev_fd != -1) { // cerrar fd previo si existe
                    close(prev_fd);
                }
                // actualizar prev_fd al extremo de lectura del pipe actual
                if (keep_going) {
                    close(pipefd[1]);
                    prev_fd = pipefd[0];
                }
            }
        }
        pipeline_pop_front(apipe); // descartar comando ya procesado
    }

    if (prev_fd != -1) {
        close(prev_fd);
    } // cerrar último fd si existe

    if (!error && pipeline_get_wait(apipe)) { // esperar a todos los hijos si corresponde
        for (int i = 0; i < total; ++i) {
            if (pids[i] > 0)
                waitpid(pids[i], NULL, 0);
        }
    }

    free(pids);
}