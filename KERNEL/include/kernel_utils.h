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
extern sem_t semaforo_planificador;
extern sem_t semaforo_ejecutando;
extern t_list *procesos;
extern pthread_mutex_t mx_procesos;
extern t_list * archivos_abiertos_global;

void planificar();
void iniciar_logger_kernel();
int iniciar_config_kernel(char*);
int iniciar_servidor_kernel();
int conectar_con_file_system();
void conectar_con_consola();
int conectar_con_cpu();
int conectar_con_memoria();
t_list* crear_recursos(char** recursos, char** instancias_recursos);

#endif