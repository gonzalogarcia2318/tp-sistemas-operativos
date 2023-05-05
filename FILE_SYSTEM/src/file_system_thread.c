#include "file_system_thread.h"
/*
void esperar_kernel(int socketFileSystem)
{
  bool desconectado = false;
  while (true)
  {

    log_info(logger, "[FILE_SYSTEM]: Esperando conexion de kernel...");
    int socket_kernel = esperar_cliente(socketFileSystem);

    if (socket_kernel < 0)
    {
      log_warning(logger, "[FILE_SYSTEM]: Kernel Desconectado.");

      return;
    }

    log_info(logger, "[FILE_SYSTEM]: Conexión de Kernel establecida.");

    desconectado = manejar_paquete_kernel(socket_kernel);
    
    if(desconectado) return;

    //Hilo hilo_kernel;
    //pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete_kernel, (void *)socket_kernel);
    //pthread_detach(hilo_kernel);
  }
}
*/
bool manejar_paquete_kernel(int socket_kernel)
{

  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_kernel))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_kernel);
      log_info(logger, "[FILE_SYSTEM]: Mensaje recibido de KERNEL: %s", mensaje);
      free(mensaje);
      break;

    case DESCONEXION:
      log_warning(logger, "[FILE_SYSTEM]: Conexión de KERNEL terminada.");
      return true;

    default:
      log_warning(logger, "[FILE_SYSTEM]: Operacion desconocida desde KERNEL.");
      break;
    }
  }
}
