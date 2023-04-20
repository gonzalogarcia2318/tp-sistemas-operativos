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
    pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete_kernel, (void *)socket_kernel);
    pthread_detach(hilo_kernel);
  }
}

void manejar_paquete_kernel(int socket_consola)
{
  
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_consola))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_consola);
      log_info(logger, "[MEMORIA]: Mensaje recibido de Kernel: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[MEMORIA]: Conexión de Kernel terminada.");
      return;

    default:
      log_warning(logger, "[MEMORIA]: Operacion desconocida desde Kernel.");
      break;
    }
  }
}