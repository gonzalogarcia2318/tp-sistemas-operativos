#include "cpu_thread.h"

PCB * recibir_pcb(int socket_kernel){

	BUFFER* buffer = recibir_buffer(socket_kernel);

	PCB* pcb = deserializar_pcb(buffer);

	free(buffer);

	return pcb;
}

void enviar_pcb(PCB* pcb)
{
  PAQUETE* paquete_pcb = crear_paquete(OP_PCB);
  paquete_pcb->buffer = serializar_pcb(pcb);
  enviar_paquete_a_cliente(paquete_pcb, socket_kernel);
  eliminar_paquete(paquete_pcb);
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
      for (int i = 0; i < list_size(pcb->instrucciones); i++)
            {
                Instruccion *instruccion = list_get(pcb->instrucciones, i);
                // log_info(logger, "Instruccion %d: nombre: %s", i, instruccion->nombreInstruccion);
            }
      log_info(logger, "[CPU]: Lista de instrucciones recibida");
      manejar_instrucciones(pcb);
      log_info(logger, "[CPU]: Esperando conexiones de KERNEL..");
      break;
    default:
      log_warning(logger, "[CPU]: Operacion desconocida desde kernel.");
      break;
    }
  }
}