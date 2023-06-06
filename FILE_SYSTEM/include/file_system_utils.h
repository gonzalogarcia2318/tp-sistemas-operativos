#ifndef FILE_SYSTEM_UTILS_H
#define FILE_SYSTEM_UTILS_H

#include "protocolo.h"
#include "file_system_config.h"
#include "file_system_thread.h"
#include <unistd.h>

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

int iniciar_logger_file_system();
int iniciar_config_file_system();
int iniciar_servidor_file_system();

int iniciar_config_superbloque();
    void rellenar_configuracion_superbloque(Config*);
int levantar_bitmap(char *path);
int iniciar_archivo_de_bloques(char*);

int conectar_con_memoria();
void conectar_con_kernel();
void terminar_ejecucion();


#endif
