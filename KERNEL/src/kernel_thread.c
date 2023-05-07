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

    case PAQUETE_2:
    //Recibir info consola
    manejar_proceso_consola();
      break;

    default:
      log_warning(logger, "[KERNEL]: Operacion desconocida desde consola.");
      break;
    }
  }
}

void manejar_proceso_consola (){

     PCB pcb;
    t_list * instrucciones = list_create();
    list_add(instrucciones,"Instruccion1");
    list_add(instrucciones,"Instruccion1");


    pcb.pID = PROCESO_ID++ ;
    pcb.instrucciones  = instrucciones ;
    pcb.program_counter = 1 ;
    //pcb.registros_cpu;  //Tipo struct REGISTROS_CPU
    //pcb.tabla_segmentos; //Lista de Struct TABLA_SEGMENTOS
    //pcb.proxima_rafaga; 
    //pcb.tiempo_ready;
    //pcb.archivos_abiertos; //Lista de struct ARCHIVOS_ABIERTOS


}

void mandar_pcb_a_cpu(PCB pcb ){

  PAQUETE * pcb = crear_paquete(CODIGO_OPERACION.PCB);

}
