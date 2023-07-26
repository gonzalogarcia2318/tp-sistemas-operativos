#ifndef CPU_THREAD_H
#define CPU_THREAD_H

#include "cpu_utils.h"

PCB* recibir_pcb(int);
void liberar_pcb_cpu(PCB* pcb);
void manejar_paquete_kernel(int socket_kernel);


#endif