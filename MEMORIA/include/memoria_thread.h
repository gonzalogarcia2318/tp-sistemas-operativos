#ifndef MEMORIA_THREAD_H
#define MEMORIA_THREAD_H

#include "memoria_utils.h"

//void esperar_kernel(int socket_memoria);
//void esperar_cpu(int socket_memoria);
//void esperar_file_system(int socket_memoria);
void escuchar_kernel(int socket_kernel);
void escuchar_file_system(int socket_fs);
void escuchar_cpu(int socket_cpu);

#endif