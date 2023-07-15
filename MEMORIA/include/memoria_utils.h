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
extern t_list* tabla_de_segmentos_globales;

typedef struct
{
    int32_t pid;
    t_list* tabla_de_segmentos;
} PROCESO_MEMORIA;

void iniciar_logger_memoria();
int iniciar_config_memoria(char*);
int iniciar_servidor_memoria();
void conectar_con_kernel();
void conectar_con_file_system();
void conectar_con_cpu();

void crear_estructuras_administrativas();
    void crear_segmento_compartido();
    void crear_espacio_usuario();
    void crear_lista_huecos_libres();
    void crear_lista_procesos_globales();
    void crear_tabla_segmentos_globales();

t_list* manejar_crear_proceso();
    t_list* crear_tabla_de_segmentos();
    void enviar_tabla_de_segmentos_a_kernel(t_list* tabla_de_segmentos);
    void enviar_tabla_de_segmentos_a_kernel_por_delete_segment(t_list* tabla_de_segmentos);
    void enviar_tablas_de_segmentos_a_kernel();
void manejar_finalizar_proceso();
    PROCESO_MEMORIA* obtener_proceso_de_globales(int32_t pid);
    void eliminar_proceso_de_globales(int32_t pid);

char* leer_de_memoria(int32_t direccion_fisica, int32_t bytes_registro);
void escribir_en_memoria(char* contenido,int32_t direccion_fisica,int32_t bytes_registro);
    void aplicar_retardo_espacio_usuario();

int32_t manejar_crear_segmento(int32_t pid, int32_t id_segmento, int32_t tamanio_segmento);
    bool puedo_crear_nuevo_segmento_proceso(t_list* tabla_de_segmentos);
    int hay_espacio_memoria(int32_t tamanio_segmento);
    int32_t crear_segmento(int32_t pid, int32_t id_segmento, int32_t tamanio_segmento);
        int32_t aplicar_algoritmo_asignacion(int32_t tamanio_segmento);
            int32_t aplicar_algoritmo_asignacion_FIRST(int32_t tamanio_segmento);
            int32_t aplicar_algoritmo_asignacion_BEST(int32_t tamanio_segmento);
            int32_t aplicar_algoritmo_asignacion_WORST(int32_t tamanio_segmento);
        void agregar_segmento_a_tabla_global(SEGMENTO*);
    t_list* obtener_tabla_de_segmentos(int32_t pid);
    SEGMENTO* obtener_segmento_de_tabla_de_segmentos(t_list* tabla_de_segmentos,int32_t id_segmento);

void manejar_eliminar_segmento(SEGMENTO* segmento);
    void redimensionar_huecos_eliminar_segmento(int32_t base_segmento, int32_t limite_segmento);
        void eliminar_hueco(int32_t base_hueco);
    void eliminar_segmento_de_globales(int32_t id_segmento);
    void eliminar_segmento_de_tabla_segmentos_proceso(int32_t pid, int32_t id_segmento);

void compactar();
    void redimensionar_huecos_compactar(int32_t base, int32_t limite);
    void imprimir_tabla_segmentos_globales();
    void aplicar_retardo_compactacion();
    void leer_y_escribir_memoria(int32_t base, SEGMENTO* segmento);

char* manejar_read_file_system();
void manejar_write_file_system();

void terminar_ejecucion_memoria();

#endif