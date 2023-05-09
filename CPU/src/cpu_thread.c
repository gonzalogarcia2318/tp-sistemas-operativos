#include "cpu_thread.h"
/*
void esperar_kernel(int socket_cpu)
{
  while (true)
  {

    log_info(logger, "[CPU]: Esperando conexiones de Kernel...");
    int socket_consola = esperar_cliente(socket_cpu);

    if (socket_consola < 0)
    {
      log_warning(logger, "[CPU]: Kernel desconectado.");

      return;
    }

    log_info(logger, "[CPU]: Conexión de Kernel establecida.");

    Hilo hilo_kernel;
    pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete_kernel, (void *)socket_consola);
    pthread_detach(hilo_kernel);
  }
}
*/

PCB * recibir_pcb(int socket_kernel){

	BUFFER* buffer = recibir_buffer(socket_kernel);

	PCB* pcb = deserializar_pcb(buffer);

	free(buffer);

	return pcb;
}

void manejar_paquete_kernel(int socket_kernel)
{
  
  while (true)
  {
    char *mensaje;
    switch (obtener_codigo_operacion(socket_kernel))
    {
    case MENSAJE:
      mensaje = obtener_mensaje_del_cliente(socket_kernel);
      log_info(logger, "[CPU]: Mensaje recibido de Kernel: %s", mensaje);
      free(mensaje);
      break;
      
    case DESCONEXION:
      log_warning(logger, "[CPU]: Conexión de Kernel terminada.");
      return;

    case OP_PCB:
      log_info(logger, "[CPU]: OP PCB Recibido de Kernel");
      PCB *pcb = recibir_pcb(socket_kernel);
      log_info(logger, "[CPU]: PCB Deserializada: con PID:[%d]",pcb->PID);
      recibir_instrucciones(pcb);
      //DEVOLVER CONTEXTO A KERNEL
      return;
        //; UTILIZA PCB y POR AHORA NO DEVUELVE NADA (void)
        //PORQUE ESTOY MANEJANDO PUNTEROS, VAMOS A MANDAR UN MENSAJE A KERNEL DICIENDO QUE 
        //EL PROCESO DE PID: X HA CONCLUIDO CON EXITO Y SE HA MODIFICADO SU CONTEXTO..
      
    default:
      log_warning(logger, "[CPU]: Operacion desconocida desde kernel.");
      break;
    }
  }  
}

