#include "cpu_thread.h"

PCB * recibir_pcb(int socket_kernel){

	BUFFER* buffer = recibir_buffer(socket_kernel);

	PCB* pcb = deserializar_pcb(buffer);

	free(buffer);

	return pcb;
}

void liberar_pcb_cpu(PCB* pcb)
{
  //--------------------------------------INSTRUCCIONES
  int size = list_size(pcb->instrucciones);

  for (int i = 0; i < size; i++)
  {
    liberar_instruccion(list_get(pcb->instrucciones,i));
  }
  list_destroy(pcb->instrucciones);
  //--------------------------------------TABLA SEGMENTOS
   
  int size2 = list_size(pcb->tabla_segmentos);

  for (int i = 0; i < size2; i++)
  {
    free(list_get(pcb->tabla_segmentos,i));
  }
  list_destroy(pcb->tabla_segmentos);
  
  //--------------------------------------PCB
   
  free(pcb);
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
     
      manejar_instrucciones(pcb);

      liberar_pcb_cpu(pcb);

      log_info(logger, "[CPU]: Esperando conexiones de KERNEL..");
      break;
    default:
      log_warning(logger, "[CPU]: Operacion desconocida desde kernel.");
      break;
    }
  }
}