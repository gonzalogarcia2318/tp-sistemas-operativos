#ifndef FILE_SYSTEM_THREAD_H
#define FILE_SYSTEM_THREAD_H

#include <file_system_utils.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "protocolo.h"
#define MAX_CARACTERES_PUNTERO 100

typedef struct
{
    char* NOMBRE_ARCHIVO;
    unsigned int  TAMANIO_ARCHIVO;
    uint32_t PUNTERO_DIRECTO;
    uint32_t PUNTERO_INDIRECTO;
} FCB;

//void esperar_kernel(int socket_file_system);
bool manejar_paquete_kernel(int socket_kernel);
void recibir_instruccion_kernel();
int crear_archivo(char* );
int existe_archivo(char*);
void ejecutar_f_truncate();
void enviar_respuesta_kernel(int, CODIGO_OPERACION);
char* obtener_info_de_memoria(int32_t , uint32_t);
int enviar_a_memoria(int32_t, char *);
int ejecutar_f_write(char *,uint32_t, uint32_t, int32_t);
int ejecutar_f_read(char *,uint32_t ,int, int);
int buscar_bloque_libre();
#endif
