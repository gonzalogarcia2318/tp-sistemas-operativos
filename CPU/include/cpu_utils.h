#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include "protocolo.h"
#include "cpu_config.h"
#include "cpu_thread.h"

#define ARCHIVO_LOGGER "config/cpu.log"
#define ARCHIVO_CONFIG "config/cpu.config"

#define SUCCESS 0
#define FAILURE -1

extern Logger *logger;
extern Config *config;
extern Hilo hilo_kernel;
extern int socket_cpu;
extern int socket_memoria;
extern int socket_cliente;
extern int socket_kernel;


void iniciar_logger_cpu();
void iniciar_config_cpu();
void iniciar_servidor_cpu();
void terminar_ejecucion();
int conectar_con_memoria();

#endif