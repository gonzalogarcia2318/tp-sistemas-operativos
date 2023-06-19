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

extern void* espacio_usuario;
extern SEGMENTO* segmento_compartido;
extern t_list* huecos_libres;
extern t_list* procesos_globales;

typedef struct
{
    int32_t pid;
    t_list* tabla_de_segmentos;
} PROCESO_MEMORIA;

void iniciar_logger_memoria();
void iniciar_config_memoria();
int iniciar_servidor_memoria();
void conectar_con_kernel();
void conectar_con_file_system();
void conectar_con_cpu();
void terminar_ejecucion();

void crear_estructuras_administrativas();
    void crear_segmento_compartido();
    void crear_espacio_usuario();
    void crear_lista_huecos_libres();
    void crear_lista_procesos_globales();

t_list* manejar_crear_proceso();
    t_list* crear_tabla_de_segmentos();
void manejar_finalizar_proceso();
    PROCESO_MEMORIA* obtener_proceso_de_globales(int32_t pid);

char* leer_de_memoria(int32_t direccion_fisica, int32_t bytes_registro);
void escribir_en_memoria(char* contenido,int32_t direccion_fisica,int32_t bytes_registro);
    void aplicar_retardo_espacio_usuario();
#endif