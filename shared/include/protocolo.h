#ifndef PROTOCOLO_H
#define PROTOCOLO_H

#include <commons/config.h>
#include <commons/log.h>
#include <commons/temporal.h>
#include <math.h>
#include <time.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/bitarray.h>
#include <commons/collections/dictionary.h>

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
    PAQUETE_CPU, // TODO: CHEQUEAR NOMBRE REDEFINIDOS ?
    OP_PCB,
    INSTRUCCION,
    INSTRUCCIONES,
    CREAR_PROCESO,
    FINALIZAR_PROCESO,
    SEG_FAULT,
    RECEPCION_OK,
    PROCESO_FINALIZADO,
    CREAR_SEGMENTO,
    BORRAR_SEGMENTO,
    CONSOLIDAR,
    SOLICITAR_COMPACTACION,
    COMPACTACION_TERMINADA,
    FALTA_MEMORIA,
    FINALIZO_TRUNCADO,
    FINALIZO_LECTURA,
    FINALIZO_ESCRITURA,
    READ,
    WRITE,
    RESPUESTA_FILE_SYSTEM
    
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
    EXIT,

    CREAR_ARCHIVO,
    EXISTE_ARCHIVO

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


// TODO: MOVER A OTRO ARCHIVO
typedef struct 
{
    char valor_AX[5];
    char valor_BX[5];
    char valor_CX[5];
    char valor_DX[5];
    char valor_EAX[9];
    char valor_EBX[9];
    char valor_ECX[9];
    char valor_EDX[9];
    char valor_RAX[17];
    char valor_RBX[17];
    char valor_RCX[17];
    char valor_RDX[17];
} Registro_CPU;


typedef struct{
    
char * nombre_archivo;
int32_t PID_en_uso;
t_queue* cola_block;

}ARCHIVO_GLOBAL;

typedef struct{

int32_t descriptor_archivo;         // Int o FILE*
char * nombre_archivo;
int32_t puntero_ubicacion;          //Ver Si long o int 

}ARCHIVO_PROCESO;


typedef struct
{
    int32_t PID;
    int32_t socket_consola;
    t_list *instrucciones;
    int32_t program_counter;        //DEBE INICIALIZARSE EN 0.
    Registro_CPU registros_cpu;     // Tipo struct REGISTROS_CPU
    t_list *tabla_segmentos;
    float estimacion_cpu_proxima_rafaga;
    time_t tiempo_ready;
    t_list * archivos_abiertos;     // Lista de struct ARCHIVO_PROCESO
    time_t tiempo_cpu_real_inicial;
    t_temporal* cronometro_ready;
    t_temporal* cronometro_exec;
    int64_t tiempo_cpu_real;
    float estimacion_cpu_anterior;
    float response_Ratio;
    t_list *recursos_asignados;
    

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
    int32_t tamanioArchivo; 
    char* recurso;
    int32_t idSegmento; //Copiar de la tabla de seg   
    int32_t nombreInstruccion_long;
    int32_t valor_long; 
    int32_t registro_long;
    int32_t recurso_long;
    int32_t nombreArchivo_long;
    int32_t tamanioSegmento;
    
} Instruccion;


typedef struct
{
    int32_t pid;
    int32_t id;
    int32_t base;
    int32_t limite;
    //int32_t validez;
} SEGMENTO;

PCB *obtener_paquete_pcb(int socket_cpu);
CODIGO_INSTRUCCION obtener_codigo_instruccion(int socket_cliente);

BUFFER *serializar_pcb(PCB *pcb);
PCB *deserializar_pcb(BUFFER *buffer);

BUFFER *serializar_instruccion(Instruccion *instruccion);
Instruccion *deserializar_instruccion(BUFFER *buffer, int stream_offset);
BUFFER *serializar_instrucciones(t_list *instrucciones);
t_list* deserializar_instrucciones(BUFFER* buffer);

BUFFER *serializar_registros(Registro_CPU *registros);
Registro_CPU *deserializar_registros(BUFFER *buffer);
int32_t obtener_tamanio_registro(char* nombre_registro);

BUFFER *serializar_segmentos(t_list *segmentos);
BUFFER *serializar_segmento(SEGMENTO *segmento);
SEGMENTO * deserializar_segmento(BUFFER* buffer, int stream_offset);
t_list * deserializar_segmentos(BUFFER* buffer);

int calcular_tamanio_instrucciones(t_list* instruccion);
int calcular_tamanio_instruccion(Instruccion *instruccion);
int calcular_tamanio_segmento(SEGMENTO *segmento);
int calcular_tamanio_segmentos(t_list *segmentos);

void imprimir_buffer( BUFFER* buffer);
void quitar_salto_de_linea(char *);

#endif