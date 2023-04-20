#ifndef CPU_CONFIG_H
#define CPU_CONFIG_H

#include <protocolo.h>

typedef struct CPU_CONFIG
{
  char *IP;
  int RETARDO_INSTRUCCION;
  char *IP_MEMORIA;
  int PUERTO_MEMORIA;
  int PUERTO_ESCUCHA;
  int TAM_MAX_SEGMENTO;
} CPU_CONFIG;

extern CPU_CONFIG CPUConfig;

void rellenar_configuracion_cpu(Config *config);

#endif
