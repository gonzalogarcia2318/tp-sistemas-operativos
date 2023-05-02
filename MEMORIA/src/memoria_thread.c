#include "memoria_thread.h"

/*
void esperar_kernel(int socket_memoria)
{
  bool desconectado = false;
  while (true)
  {

    log_info(logger, "[MEMORIA]: Esperando conexiones de Kernel...");
    int socket_kernel = esperar_cliente(socket_memoria);

    if (socket_kernel < 0)
    {
      log_warning(logger, "[MEMORIA]: Kernel desconectado.");

      return;
    }

    

    desconectado = manejar_paquete_kernel(socket_kernel);
    
    if(desconectado) return;
    
    //Hilo hilo_kernel_2;
    //pthread_create(&hilo_kernel_2, NULL, (void *)manejar_paquete, (void *)socket_kernel);
    //pthread_detach(hilo_kernel_2);
  }
}

void esperar_cpu(int socket_memoria)
{
  while (true)
  {

    log_info(logger, "[MEMORIA]: Esperando conexiones de CPU...");
    int socket_cpu = esperar_cliente(socket_memoria);

    if (socket_cpu < 0)
    {
      log_warning(logger, "[MEMORIA]: CPU desconectado.");

      return;
    }

    log_info(logger, "[MEMORIA]: Conexión de CPU establecida.");

    //Hilo hilo_cpu_2;
    //pthread_create(&hilo_cpu_2, NULL, (void *)manejar_paquete_cpu, (void *)socket_cpu);
    //pthread_detach(hilo_cpu_2);
  }
}

void esperar_file_system(int socket_memoria)
{
  bool desconectado = false;

  while (true)
  {

    log_info(logger, "[MEMORIA]: Esperando conexiones de FILE SYSTEM...");
    int socket_fs = esperar_cliente(socket_memoria);

    if (socket_fs < 0)
    {
      log_warning(logger, "[MEMORIA]: FILE SYSTEM desconectado.");

      return;
    }

    log_info(logger, "[MEMORIA]: Conexión de FILE SYSTEM establecida.");

    desconectado = manejar_paquete_fs(socket_file_system);

    if(desconectado) return;
    
    //Hilo hilo_fs;
    //pthread_create(&hilo_fs, NULL, (void *)manejar_paquete, (void *)socket_fs);
    //pthread_detach(hilo_fs,NULL);
  }
}
*/
void escuchar_kernel(int socket_kernel)
{
  log_info(logger,"[MEMORIA]: Escuchando KERNEL...");
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_kernel))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_kernel);
      log_info(logger, "[MEMORIA]: Mensaje recibido de KERNEL: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[MEMORIA]: Conexión con KERNEL terminada.");
      return;

    default:
      log_warning(logger, "[MEMORIA]: Operacion desconocida.");
      break;
    }
  }
}

void escuchar_file_system(int socket_fs)
{
  log_info(logger,"[MEMORIA]: Escuchando FILE SYSTEM...");
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_fs))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_fs);
      log_info(logger, "[MEMORIA]: Mensaje recibido de FILE SYSTEM: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[MEMORIA]: Conexión con FILE SYSTEM terminada.");
      return;

    default:
      log_warning(logger, "[MEMORIA]: Operacion desconocida.");
      break;
    }
  }
}
void escuchar_cpu(int socket_cpu)
{
  log_info(logger,"[MEMORIA]: Escuchando CPU...");
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_cpu))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_cpu);
      log_info(logger, "[MEMORIA]: Mensaje recibido de CPU: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[MEMORIA]: Conexión con CPU terminada.");
      return;

    default:
      log_warning(logger, "[MEMORIA]: Operacion desconocida.");
      break;
    }
  }
}