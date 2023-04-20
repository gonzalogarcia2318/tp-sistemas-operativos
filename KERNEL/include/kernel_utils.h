#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H

#include <protocolo.h>
#include <kernel_config.h>
#include <kernel_thread.h>

#define ARCHIVO_LOGGER "kernel.log"
#define ARCHIVO_CONFIG "kernel.config"

extern Logger *logger;
extern Config *config;
extern Hilo hilo_consolas;
extern int socket_kernel;
extern int socket_cpu;

void iniciar_logger_kernel();
void iniciar_config_kernel();
void iniciar_servidor_kernel();
void conectar_con_consola();
void conectar_con_cpu();
void terminar_ejecucion();

#endif