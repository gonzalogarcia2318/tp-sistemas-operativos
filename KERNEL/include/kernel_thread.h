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
    PCB * pcb;
    ESTADO estado;
} Proceso;

void esperar_consola(int socket_kernel);
void manejar_paquete_consola(int socket_consola);
void manejar_proceso_consola();
void enviar_pcb_a_cpu(PCB *);

#endif