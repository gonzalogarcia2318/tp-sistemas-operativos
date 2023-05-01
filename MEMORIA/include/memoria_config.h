#ifndef MEMORIA_CONFIG_H
#define MEMORIA_CONFIG_H

#include <protocolo.h>

typedef struct MEMORIA_CONFIG
{
  char *IP;
  char *PUERTO_ESCUCHA;
  int TAM_MEMORIA;
  int TAM_SEGMENTO_0;
  int CANT_SEGMENTOS;
  int RETARDO_MEMORIA;
  int RETARDO_COMPACTACION;
  char *ALGORITMO_ASIGNACION;
} MEMORIA_CONFIG;

extern MEMORIA_CONFIG MemoriaConfig;

void rellenar_configuracion_memoria(Config *config);

#endif
