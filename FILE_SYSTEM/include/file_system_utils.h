#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include <protocolo.h>
#include <file_system_config.h>
#include <file_system_thread.h>

#define ARCHIVO_LOGGER "fileSystem.log"
#define ARCHIVO_CONFIG "fileSystem.config"

extern Logger *logger;
extern Config *config;
extern Hilo hilo_fileSystem;
extern int socket_fileSystem;

void iniciar_logger_fileSystem();
void iniciar_config_fileSystem();
void iniciar_servidor_fileSystem();
void conectar_con_kernel();
void terminar_ejecucion();

#endif
