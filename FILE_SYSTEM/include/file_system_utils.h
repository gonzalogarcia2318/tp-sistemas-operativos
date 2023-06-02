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

typedef struct
{
    int32_t BLOCK_SIZE;
    int32_t BLOCK_COUNT;
} SUPERBLOQUE;


extern Logger *logger;
extern Config *config;
extern Config *config_superbloque;
extern Hilo hilo_fileSystem;
extern int socket_file_system;
extern int socket_memoria;
extern int socket_kernel;
extern SUPERBLOQUE superbloque;

void iniciar_logger_file_system();
void iniciar_config_file_system();
void iniciar_config_superbloque();

int iniciar_servidor_file_system();
int levantar_bitmap(char *path);
void conectar_con_memoria();
void conectar_con_kernel();
void terminar_ejecucion();
void rellenar_configuracion_superbloque(Config*);

#endif
