#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <commons/config.h>
#include <commons/log.h>
#include <math.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

// #include <list.h>
#include <commons/string.h>
#include <pthread.h>
#include <semaphore.h>
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
    PAQUETE_2, // TODO: CHEQUEAR NOMBRE REDEFINIDOS ?
    OP_PCB,
    INSTRUCCION,
    INSTRUCCIONES,
    FINALIZAR_PROCESO

} CODIGO_OPERACION;

typedef enum
{
    MOV_IN,
    MOV_OUT,
    IO,
    F_OPEN,
    F_CLOSE,
    F_SEEK,
    F_READ,
    F_WRITE,
    F_TRUNCATE,
    WAIT,
    SIGNAL,
    CREATE_SEGMENT,
    DELETE_SEGMENT,
    YIELD,
    EXIT
} CODIGO_INSTRUCCION;
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
void * obtener_paquete_estructura_dinamica(int socketCliente);

BUFFER* recibir_buffer(int socket);

void *obtener_paquete_pcb(int socket_cpu);
CODIGO_INSTRUCCION obtener_codigo_instruccion(int socket_cliente);


// TODO: MOVER A OTRO ARCHIVO
typedef struct 
{
    char* valor_AX;
    char* valor_BX;
    char* valor_CX;
    char* valor_DX;
    char* valor_EAX;
    char* valor_EBX;
    char* valor_ECX;
    char* valor_EDX;
    char* valor_RAX;
    char* valor_RBX;
    char* valor_RCX;
    char* valor_RDX;
} Registro_CPU;

typedef struct
{
    int32_t PID;
    t_list *instrucciones;
    int32_t program_counter; //DEBE INICIALIZARSE EN 0.
    Registro_CPU *registros_cpu;   // Tipo struct REGISTROS_CPU
    char *tabla_segmentos; // Lista de Struct TABLA_SEGMENTOS
    double proxima_rafaga;
    char *tiempo_ready;
    char *archivos_abiertos; // Lista de struct ARCHIVOS_ABIERTOS
} PCB;

typedef struct 
{
    char* nombreInstruccion;
    char* valor; // TODO: Chequear. SET AX HOLA
    char* registro; //Recibe nombr de registro, comparo y asigno al registro del PCB
    int32_t direccionLogica;
    int32_t direccionFisica;
    int32_t tiempo;
    char* nombreArchivo;
    int32_t posicion;
    int32_t cantBytes;
    char* recurso;
    int32_t idSegmento;    
    
    int32_t nombreInstruccion_long;
    int32_t valor_long; 
    int32_t registro_long;
    int32_t recurso_long;
    int32_t nombreArchivo_long;
    int32_t tamanioSegmento;
    
} Instruccion;


BUFFER *serializar_pcb(PCB *pcb);
PCB *deserializar_pcb(BUFFER *buffer);

BUFFER *serializar_instruccion(Instruccion *instruccion);
Instruccion *deserializar_instruccion(BUFFER *buffer, int stream_offset);
BUFFER *serializar_instrucciones(t_list *instrucciones);
t_list* deserializar_instrucciones(BUFFER* buffer);

void imprimir_buffer( BUFFER* buffer);

#endif