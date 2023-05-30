#include "memoria_thread.h"

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

    case INSTRUCCION:
      log_info(logger, "[MEMORIA]: INSTRUCCION recibida de KERNEL");
      recibir_instruccion_kernel();
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
    case INSTRUCCION:
      log_info(logger, "[MEMORIA]: INSTRUCCION recibida de CPU");
      recibir_instruccion_cpu();
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

void recibir_instruccion_cpu()
{
  int direccion_fisica = 0;
  Lista* lista_recepcion;
  lista_recepcion = obtener_paquete_como_lista(socket_cpu);

  CODIGO_INSTRUCCION cod_instruccion = *(CODIGO_INSTRUCCION *)list_get(lista_recepcion,0);

  switch (cod_instruccion)
  {
    case MOV_IN:
      log_info(logger, "[MEMORIA]: INSTRUCCION recibida: MOV_IN");
      direccion_fisica = *(int*)list_get(lista_recepcion,1);
      //...
      //leer de memoria y devolver valor de la dire_fisica dada
      //...TODO
      enviar_mensaje_a_cliente("VALOR_LEIDO",socket_cpu);
      log_info(logger, "[MEMORIA]: MENSAJE ENVIADO A CPU: <VALOR_LEIDO> COMO MOTIVO DE FIN DE MOV_IN");
      break;

    case MOV_OUT:
      log_info(logger, "[MEMORIA]: INSTRUCCION recibida: MOV_OUT");
      direccion_fisica = *(int*)list_get(lista_recepcion,1);
      char* valor_a_escribir = string_duplicate((char*)list_get(lista_recepcion,2));
      //...
      //escribir valor en memoria
      //...TODO
      enviar_mensaje_a_cliente("OK",socket_cpu);
      log_info(logger,"MEMORIA: ENVIE EL MENSAJE <OK> A CPU COMO MOTIVO DE FIN DE MOV_OUT");
      break;
    
  default:
    log_warning(logger, "[MEMORIA]: Código Instrucción desconocido.");
    break;
  }
  
  list_destroy(lista_recepcion);
}

void recibir_instruccion_kernel()
{
  Lista* lista_recepcion;
  lista_recepcion = obtener_paquete_como_lista(socket_kernel);
  
  CODIGO_INSTRUCCION cod_instruccion = *(CODIGO_INSTRUCCION *)list_get(lista_recepcion,0);
  int pid = *(int*)list_get(lista_recepcion,1);
  int id_segmento = *(int*)list_get(lista_recepcion,2);
  int dire_base = *(int*)list_get(lista_recepcion,3);
  int tamanio = *(int*)list_get(lista_recepcion,4);
  
  switch (cod_instruccion)
  {
    case CREATE_SEGMENT:
      
      log_info(logger,"CREAR SEGMENTO: <PID: %d> - Crear Segmento: <ID SEGMENTO: %d> - Base: <DIRECCION BASE: %d> - tamaño: <TAMAÑO: %d>",
                      pid,
                      id_segmento,
                      dire_base,
                      tamanio
              );
                   
      //ejecutar_create_segment(...); TODO

      enviar_mensaje_a_cliente("CREATE_SEGMENT: OK", socket_kernel);
      break;

    case DELETE_SEGMENT:
      
      log_info(logger, "ELIMINAR SEGMENTO: <PID: %d> - Eliminar Segmento: <ID SEGMENTO: %d> - Base: <DIRECCION BASE: %d> - tamaño: <TAMAÑO: %d>",
                      pid,
                      id_segmento,
                      dire_base,
                      tamanio
              );

      //ejecutar_delete_segment(...); TODO

      enviar_mensaje_a_cliente("DELETE_SEGMENT: OK", socket_kernel);
      break;

    default:
      log_warning(logger, "[MEMORIA]: Código Instrucción desconocido.");
      break;
  }

  list_destroy(lista_recepcion);
}