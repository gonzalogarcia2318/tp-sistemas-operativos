#ifndef KERNEL_THREAD_H
#define KERNEL_THREAD_H

#include "kernel_utils.h"

typedef enum
{
    NEW,
    READY,
    EXEC,
    BLOCK,
    FINISHED
} ESTADO;

typedef struct
{
    PCB *pcb;
    ESTADO estado;
} Proceso;

typedef struct
{
    int32_t PID;
    int tiempo_bloqueado;
} Proceso_IO;

typedef struct
{
    char *nombre;
    int instancias;
    t_queue *cola_block;
} Recurso;

void esperar_consola(int socket_kernel);
void manejar_paquete_consola(int socket_consola);
void manejar_proceso_consola();
void enviar_pcb_a_cpu(PCB *);
void confirmar_recepcion_a_consola(int socket_consola);

Proceso *obtener_proceso_por_pid(int32_t PID);

#endif