#ifndef KERNEL_CONFIG_H
#define KERNEL_CONFIG_H

#include <protocolo.h>

typedef struct KERNEL_CONFIG
{
    char *IP;
    char *PUERTO_ESCUCHA;
    char *IP_CPU;
    char *PUERTO_CPU;
    char *IP_MEMORIA;
    char *PUERTO_MEMORIA;
    char *IP_FILESYSTEM;
    char *PUERTO_FILESYSTEM;
    char *ALGORITMO_PLANIFICACION;
    char *ESTIMACION_INICIAL;
    char *HRRN_ALFA;
    char *GRADO_MAX_MULTIPROGRAMACION;
    char **RECURSOS;
    char **INSTANCIAS_RECURSOS;
} KERNEL_CONFIG;

extern KERNEL_CONFIG KernelConfig;

void rellenar_configuracion_kernel(Config *config);

#endif