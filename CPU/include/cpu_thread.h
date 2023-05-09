#ifndef CPU_THREAD_H
#define CPU_THREAD_H

#include "cpu_utils.h"

//void esperar_kernel(int socket_cpu);
void manejar_paquete_kernel(int socket_kernel);
//Cambiar de void a estructura de PCB //todo
void manejar_instruccion();

#endif