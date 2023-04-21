#include <file_system_thread.h>

void esperar_kernel(int socketFileSystem)
{
  while (true)
  {

    log_info(logger, "[FILE_SYSTEM]: Esperando conexion de kernel...");
    int socketkernel = esperar_cliente(socketFileSystem);

    if (socketkernel < 0)
    {
      log_warning(logger, "[FILE_SYSTEM]: Kernel Desconectado.");

      return;
    }

    log_info(logger, "[FILE_SYSTEM]: Conexión de Kernel establecida.");

    Hilo hilo_kernel;
    pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete_kernel, (void *)socketkernel);
    pthread_detach(hilo_kernel);
  }
}

void manejar_paquete_kernel(int socket_file_system)
{

  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_file_system))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_file_system);
      log_info(logger, "[FILE_SYSTEM]: Mensaje recibido de Kernel: %s", mensaje);
      free(mensaje);
      break;

    case DESCONEXION:
      log_warning(logger, "[FILE_SYSTEM]: Conexión de Kernel terminada.");
      return;

    default:
      log_warning(logger, "[FILE_SYSTEM]: Operacion desconocida desde Kernel.");
      break;
    }
  }
}
