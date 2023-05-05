#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include "protocolo.h"
#include "file_system_config.h"
#include "file_system_thread.h"

#define ARCHIVO_LOGGER "config/fileSystem.log"
#define ARCHIVO_CONFIG "config/file_system.config"
#define SUCCESS 0
#define FAILURE -1

#define SUCCESS 0
#define FAILURE -1

extern Logger *logger;
extern Config *config;
extern Hilo hilo_fileSystem;
extern int socket_file_system;
extern int socket_memoria;
extern int socket_kernel;

void iniciar_logger_file_system();
void iniciar_config_file_system();
int iniciar_servidor_file_system();
void conectar_con_memoria();
void conectar_con_kernel();
void terminar_ejecucion();

#endif
