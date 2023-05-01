#ifndef CONSOLA_CONFIG_H
#define CONSOLA_CONFIG_H

#include "protocolo.h"

typedef struct CONSOLA_CONFIG
{
  char *IP_KERNEL;
  char *PUERTO_KERNEL;
} CONSOLA_CONFIG;

extern CONSOLA_CONFIG ConsolaConfig;

void rellenar_configuracion_consola(Config *config);

#endif