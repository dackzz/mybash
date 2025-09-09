#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <glib.h>

#include "command.h"
#include "strextra.h"

struct scommand_s {
    GQueue *args;
    char *in;
    char *out;
};

scommand scommand_new(void){
    scommand self = malloc(sizeof(struct scommand_s));
    if (self != NULL) {
        self->args = g_queue_new();     //inicializa la cola de argumentos
        self->in = NULL;                //inicializa la redirección de entrada
        self->out = NULL;               //inicializa la redirección de salida
    }
    return self;
}

scommand scommand_destroy(scommand self){
    assert (self != NULL);

    if (self->args != NULL) {
        g_queue_free_full(self->args, free);   // libera la cola y todas las cadenas dentro de ella
    } 
    free (self->in);    // libera la cadena de redirección de entrada
    free (self->out);   // libera la cadena de redirección de salida
    free (self);       // libera el struct en si mismo

    return NULL;
}

void scommand_push_back(scommand self, char * argument){
    assert (self != NULL && argument != NULL);
    g_queue_push_tail (self->args, argument);   // crea la copia del argumento y lo agrega al final de la cola
}

void scommand_pop_front(scommand self){
    assert (self != NULL && !scommand_is_empty (self));
    free (g_queue_pop_head (self->args));   // libera la cadena que estaba al frente de la cola y la saca de la cola
}

void scommand_set_redir_in(scommand self, char * filename){
    assert (self != NULL);
    free (self->in);    // libera la redirección de entrada anterior
    self->in = NULL;    

    if (filename != NULL){
        self->in = filename;    // crea la copia del filename y redirecciona
    }
}

void scommand_set_redir_out(scommand self, char * filename){
    assert (self != NULL);
    free (self->out);   // libera la redirección de salida anterior
    self->out = NULL;

    if (filename != NULL) {
        self->out =filename;   // crea la copia del filename y redirecciona
    }
}

bool scommand_is_empty(const scommand self){
    assert(self != NULL);
    return g_queue_is_empty(self->args);    //devuelve true si la cola de argumentos está vacía
}

unsigned int scommand_length(const scommand self){
    assert(self != NULL);
    return g_queue_get_length(self->args);  //devuelve la cantidad de elementos en la cola de argumentos
}

char * scommand_front(const scommand self){
    assert(self != NULL && !scommand_is_empty(self));
    char * result = g_queue_peek_head(self->args);  //devuelve el primer elemento de la cola de argumentos sin sacarlo
    return result;
}

char * scommand_get_redir_in(const scommand self){
    assert(self != NULL);
    return self->in;    //devuelve la cadena de redirección de entrada (o NULL si no hay redirección)
}

char * scommand_get_redir_out(const scommand self){
    assert(self != NULL);
    return self->out;   //devuelve la cadena de redirección de salida (o NULL si no hay redirección)
}

char * scommand_to_string(const scommand self) {
    assert(self != NULL);
    char * arg = calloc(1, sizeof(char)); // representación temporal del comando simple

    for (guint i = 0; i < g_queue_get_length(self->args); i++) {
        char * current_arg = g_queue_peek_nth(self->args, i);
        char * tmp = strmerge(arg, current_arg); // agrega el argumento actual a la representación
        free(arg);
        arg = tmp;
        if (i < g_queue_get_length(self->args) - 1) { // 
            tmp = strmerge(arg, " "); // agrega un espacio solo entre argumentos
            free(arg);
            arg = tmp;
        }
    }

    if (self->out != NULL) {
        char * tmp = strmerge(arg, " > ");   // separa la redirección de salida con un espacio y el símbolo >
        free(arg);
        arg = tmp;
        tmp = strmerge(arg, self->out);   // agrega la cadena de redirección de salida a la representación
        free(arg);
        arg = tmp;
    }

    if (self->in != NULL) {
        char * tmp = strmerge(arg, " < ");   // separa la redirección de entrada con un espacio y el símbolo <
        free(arg);
        arg = tmp;
        tmp = strmerge(arg, self->in);    // agrega la cadena de redirección de entrada a la representación
        free(arg);
        arg = tmp;
    }


    return arg;
}

struct pipeline_s { 
    GSList * comms; 
    bool wait; 
};

pipeline pipeline_new(void){
    pipeline p =(pipeline)malloc(sizeof(struct pipeline_s));
    if (p != NULL) {
        p->comms = NULL;    
        p->wait = true;     //por defecto, el pipeline espera     
    }
    return p;               //si no hay memoria suficiente, devuelve NULL directamente
}

pipeline pipeline_destroy(pipeline self){
    assert(self != NULL);

    g_slist_free_full(self->comms, free); // libera cada scommand correctamente
    free(self);

    return NULL;
}

void pipeline_push_back(pipeline self, scommand sc) {
    assert(self != NULL && sc != NULL);

    self->comms = g_slist_append(self->comms, sc); //self->comms apunta a la lista con el comando agregado
}

void pipeline_pop_front(pipeline self){
    assert(self!=NULL && !pipeline_is_empty(self));

    scommand_destroy(self->comms->data);    //destruye el comando simple que está en el head de la lista
    GSList *head = self->comms;  //apunta al head de la lista
    self->comms = g_slist_delete_link(self->comms, head);   //self->comms apunta a la lista sin el comando que estaba en el head
}

void pipeline_set_wait(pipeline self, const bool w) {
    assert(self != NULL);

    self->wait = w;     // si w es true, el pipeline debe esperar; si es false, no debe esperar
}

bool pipeline_is_empty(const pipeline self) {
    assert(self != NULL);   
    return self->comms == NULL;
}

unsigned int pipeline_length(const pipeline self) {
    assert(self != NULL);
    return g_slist_length(self->comms);
}

scommand pipeline_front(const pipeline self) {
    assert(self != NULL && !pipeline_is_empty(self));
    return g_slist_nth_data(self->comms, 0);
}

bool pipeline_get_wait(const pipeline self) {
    assert(self != NULL);
    return self->wait;      //devuelve true si el pipeline debe esperar, false si no debe esperar
}

char * pipeline_to_string(const pipeline self){ 
    assert(self != NULL);

    if (pipeline_is_empty(self)) {
        return g_strdup(self->wait ? "" : "&");
    }

    char * res = calloc(1, sizeof(char)); // inicializa cadena vacía

    for (GSList * i = self->comms; i != NULL; i = i->next) {     
        scommand sc = i->data;
        char * sc_str = scommand_to_string(sc); 

        char * tmp = strmerge(res, (sc_str && sc_str[0] != '\0') ? sc_str : "<empty-cmd>");
        free(res);
        res = tmp;
        free(sc_str);

        if (i->next != NULL) {
            tmp = strmerge(res, " | ");
            free(res);
            res = tmp;
        }
    }

    if (!self->wait) {
        char * tmp = strmerge(res, " &");
        free(res);
        res = tmp;
    }

    return res;   
}