#ifndef MEMORIA_THREAD_H
#define MEMORIA_THREAD_H

#include "memoria_utils.h"

void esperar_kernel(int socket_memoria);
void esperar_cpu(int socket_memoria);
void manejar_paquete(int socket_kernel);

#endif