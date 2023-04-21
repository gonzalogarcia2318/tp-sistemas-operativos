#ifndef FILE_SYSTEM_THREAD_H
#define FILE_SYSTEM_THREAD_H

#include <file_system_utils.h>

void esperar_kernel(int socket_file_system);
void manejar_paquete_kernel(int socket_kernel);

#endif
