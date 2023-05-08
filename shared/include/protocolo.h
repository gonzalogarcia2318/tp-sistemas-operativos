#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <commons/config.h>
#include <commons/log.h>
// #include <list.h>
#include <string.h>
#include <pthread.h>
#include "cliente.h"
#include "server.h"
#include "socket.h"
#include "tipos.h"



typedef t_config Config;
typedef t_list Lista;
typedef t_log Logger;
typedef pthread_t Hilo;

typedef enum
{
    DESCONEXION = -1,
    MENSAJE,
    PAQUETE_2,
    OP_PCB, // TODO: CHEQUEAR NOMBRE REDEFINIDOS ?
    FINALIZAR_PROCESO
} CODIGO_OPERACION;

typedef struct
{
    int size;
    void *stream;
} BUFFER;

typedef struct
{
    CODIGO_OPERACION codigo_operacion;
    BUFFER *buffer;
} PAQUETE;

// ------- Funciones de Cliente y Servidor  -------

PAQUETE *crear_paquete(CODIGO_OPERACION codigoOperacion);
void eliminar_paquete(PAQUETE *paquete);
void inicializar_buffer(PAQUETE *paquete);
void agregar_a_paquete(PAQUETE *paquete, void *valor, int tamanio);
void *serializar_paquete(PAQUETE *paquete, int bytes);

void enviar_paquete_a_cliente(PAQUETE *paquete, int socketCliente);
void enviar_mensaje_a_cliente(char *mensaje, int socketCliente);

void enviar_paquete_a_servidor(PAQUETE *paquete, int socketCliente);
void enviar_mensaje_a_servidor(char *mensaje, int socketCliente);

CODIGO_OPERACION obtener_codigo_operacion(int socketCliente);
void *obtener_buffer_del_cliente(int *tamanio, int socketCliente);
char *obtener_mensaje_del_cliente(int socketCliente);
Lista *obtener_paquete_como_lista(int socketCliente);
char *obtener_mensaje_del_servidor(int socketServidor);

BUFFER* recibir_buffer(int socket);


// TODO: MOVER A OTRO ARCHIVO
typedef struct
{
    int32_t pID;
    t_list *instrucciones;
    int32_t program_counter;
    char *registros_cpu;   // Tipo struct REGISTROS_CPU
    char *tabla_segmentos; // Lista de Struct TABLA_SEGMENTOS
    double proxima_rafaga;
    char *tiempo_ready;
    char *archivos_abiertos; // Lista de struct ARCHIVOS_ABIERTOS
} PCB;

BUFFER *serializar_pcb(PCB *pcb);
PCB *deserializar_pcb(BUFFER *buffer);

#endif