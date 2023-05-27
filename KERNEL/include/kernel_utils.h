#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include "protocolo.h"
#include "kernel_config.h"
#include "kernel_thread.h"

#define ARCHIVO_LOGGER "config/kernel.log"
#define ARCHIVO_CONFIG "config/kernel.config"

#define SUCCESS 0
#define FAILURE -1

extern Logger *logger;
extern Config *config;
extern Hilo hilo_consolas;
extern int socket_kernel;
extern int socket_cpu;
extern int socket_memoria;
extern int socket_file_system;
extern sem_t semaforo_new;
extern t_list *procesos;

void planificar();
void iniciar_logger_kernel();
void iniciar_config_kernel();
int iniciar_servidor_kernel();
void conectar_con_file_system();
void conectar_con_consola();
void conectar_con_cpu();
void conectar_con_memoria();
void terminar_ejecucion();

#endif