#ifndef MEMORIA_UTILS_H
#define MEMORIA_UTILS_H

#include "protocolo.h"
#include "memoria_config.h"
#include "memoria_thread.h"

#define ARCHIVO_LOGGER "config/memoria.log"
#define ARCHIVO_CONFIG "config/memoria.config"

extern Logger *logger;
extern Config *config;
extern Hilo hilo;
extern Hilo hilo_cpu;
extern int socket_memoria;
extern int socket_kernel;

void iniciar_logger_memoria();
void iniciar_config_memoria();
void iniciar_servidor_memoria();
void conectar_con_kernel();
void terminar_ejecucion();

#endif