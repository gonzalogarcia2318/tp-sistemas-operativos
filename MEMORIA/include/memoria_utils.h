#ifndef MEMORIA_UTILS_H
#define MEMORIA_UTILS_H

#include "protocolo.h"
#include "memoria_config.h"
#include "memoria_thread.h"

#define ARCHIVO_LOGGER "config/memoria.log"
#define ARCHIVO_CONFIG "config/memoria.config"

#define SUCCESS 0
#define FAILURE -1

extern Logger *logger;
extern Config *config;
extern Hilo hilo_kernel;
extern Hilo hilo_cpu;
extern Hilo hilo_file_system;
extern int socket_memoria;
extern int socket_kernel;
extern int socket_file_system;
extern int socket_cpu;

void iniciar_logger_memoria();
void iniciar_config_memoria();
int iniciar_servidor_memoria();
void conectar_con_kernel();
void conectar_con_file_system();
void conectar_con_cpu();
void terminar_ejecucion();

#endif