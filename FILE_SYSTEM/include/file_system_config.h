#ifndef FILE_SYSTEM_CONFIG_H
#define FILE_SYSTEM_CONFIG_H

#include <protocolo.h>

typedef struct FILE_SYSTEM_CONFIG
{
    char *IP;
    char *PUERTO_MEMORIA;
    char *PUERTO_ESCUCHA;
    char *PATH_SUPERBLOQUE;
    char *PATH_BITMAP;
    char *PATH_BLOQUES;
    char *PATH_FCB;
    char *RETARDO_ACCESO_BLOQUE;

} FILE_SYSTEM_CONFIG;

extern FILE_SYSTEM_CONFIG FileSystemConfig;

void rellenar_configuracion_file_system(Config *config);

#endif
