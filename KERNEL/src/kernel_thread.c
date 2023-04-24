#include "kernel_thread.h"

void esperar_consola(int socketKernel)
{
  while (true)
  {

    log_info(logger, "[KERNEL]: Esperando conexiones de Consola...");
    int socketConsola = esperar_cliente(socketKernel);

    if (socketConsola < 0)
    {
      log_warning(logger, "[KERNEL]: Consola desconectada.");

      return;
    }

    log_info(logger, "[KERNEL]: Conexión de Consola establecida.");

    Hilo hilo_consola;
    pthread_create(&hilo_consola, NULL, (void *)manejar_paquete_consola, (void *)socketConsola);
    pthread_detach(hilo_consola);
  }
}

void manejar_paquete_consola(int socketConsola)
{
  
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socketConsola))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socketConsola);
      log_info(logger, "[KERNEL]: Mensaje recibido de Consola: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[KERNEL]: Conexión de Consola terminada.");
      return;

    default:
      log_warning(logger, "[KERNEL]: Operacion desconocida desde consola.");
      break;
    }
  }
}