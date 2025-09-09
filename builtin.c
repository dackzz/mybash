#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tests/syscall_mock.h"
#include "builtin.h"
#include "command.h"
#include "strextra.h"

// Lista de comandos internos reconocidos por el programa
static const char* builtin_cmds[] = {"cd" , "help", "exit"};

// Cantidad total de comandos internos
static const unsigned int builtin_size = 3;

// Verifica si el comando recibido en cmd corresponde a un comando interno
bool builtin_is_internal(scommand cmd) {
    assert(cmd != NULL); 
    
    // Recorre la lista de comandos internos
    for (unsigned int i = 0; i < builtin_size; i++) {
        // Compara el primer elemento de cmd con cada comando interno
        if (!strcmp(scommand_front(cmd), builtin_cmds[i])) {
            return true;
        }
    }
    
    return false; // Si no coincide con ninguno, no es un comando interno
}


// Verifica si el pipeline contiene un único comando y si dicho comando es interno
bool builtin_alone(pipeline p) {
    assert(p != NULL);
    
    // Devuelve true solo si:
    // 1) El pipeline tiene exactamente un comando
    // 2) Ese comando pertenece a la lista de comandos internos
    return (pipeline_length(p) == 1) && builtin_is_internal(pipeline_front(p));
}


// Ejecuta un comando interno (cd, help o exit)
void builtin_run(scommand cmd) {
    assert(builtin_is_internal(cmd));
    
    // Caso: comando "cd"
    if (!strcmp(scommand_front(cmd), builtin_cmds[0])) {
        scommand_pop_front(cmd); // Quita "cd" y deja solo el posible argumento
        
        // Si no se especifica directorio, se redirige al home del usuario
        if (scommand_length(cmd) == 0) {
            char* user_name = getenv("USER");           // Obtiene el nombre del usuario
            char* home_dir = strmerge("/home/", user_name); // Construye la ruta /home/usuario
            scommand_push_back(cmd, home_dir);          // Inserta la ruta como argumento
        }

        // Intenta cambiar al directorio indicado
        int cd = chdir(scommand_front(cmd));  
        
        // Si falla, muestra un mensaje de error en stderr
        if (cd == -1) { 
            fprintf(stderr, "cd: cannot access '%s': %s\n",
                scommand_front(cmd), strerror(errno));
        }
    
    // Caso: comando "help"
    } else if (!strcmp(scommand_front(cmd), builtin_cmds[1])) {
        printf("Bash created by Facundo Mauvecin, Patricio Rivadeneira and Isabelle Costa.\n"
               "In this bash, you can use the following commands:\n"
               "cd <pathname>   - To change directories\n"
               "exit            - To exit bash\n"
               "help            - To see how the commands work\n");
    
    // Caso: comando "exit"
    } else {
        exit(EXIT_SUCCESS); // Finaliza el programa con éxito
    } 
}
