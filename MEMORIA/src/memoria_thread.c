#include <memoria_thread.h>

void esperar_kernel(int socket_memoria)
{
  while (true)
  {

    log_info(logger, "[MEMORIA]: Esperando conexiones de Kernel...");
    int socket_kernel = esperar_cliente(socket_memoria);

    if (socket_kernel < 0)
    {
      log_warning(logger, "[MEMORIA]: Kernel desconectado.");

      return;
    }

    log_info(logger, "[MEMORIA]: Conexión de Kernel establecida.");

    Hilo hilo_kernel;
    pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete, (void *)socket_kernel);
    pthread_detach(hilo_kernel);
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

    Hilo hilo_cpu;
    pthread_create(&hilo_cpu, NULL, (void *)manejar_paquete, (void *)socket_cpu);
    pthread_detach(hilo_cpu);
  }
}

void manejar_paquete(int socket_ciente)
{
  
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_ciente))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_ciente);
      log_info(logger, "[MEMORIA]: Mensaje recibido: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[MEMORIA]: Conexión terminada.");
      return;

    default:
      log_warning(logger, "[MEMORIA]: Operacion desconocida.");
      break;
    }
  }
}
