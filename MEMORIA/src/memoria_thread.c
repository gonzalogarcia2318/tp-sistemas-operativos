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
    case CREAR_PROCESO:
      int32_t pid; // ¿Como llega?
      log_info(logger, "CREACIÓN DE PROCESO: PID<%d>", pid);
      t_list* tabla_de_segmentos = crear_tabla_de_segmentos();
      //DEVOLVER TABLA DE SEGMENTOS A KERNEL.

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
    
    case INSTRUCCION:
      log_warning(logger,"[MEMORIA]: INSTRUCCION recibida de FILE SYSTEM");
      recibir_instruccion_file_system();

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
  BUFFER *buffer = recibir_buffer(socket_cpu);

  int cod_instruccion;
  int32_t pid;
  int32_t direccion_fisica;
  int32_t tamanio_registro;
  
  memcpy(&cod_instruccion, buffer->stream + sizeof(int32_t), sizeof(int));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
  memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
  memcpy(&direccion_fisica, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
  memcpy(&tamanio_registro, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

  log_info(logger,"INSTRUCCIÓN CPU: COD:<%d> - PID:<%d> - DF:<%d> - TR: <%d>",
            cod_instruccion,
            pid,
            direccion_fisica,
            tamanio_registro
          );

  switch (cod_instruccion)
  {
    case MOV_IN: //TODO
      log_info(logger, "[MEMORIA]: INSTRUCCION recibida: MOV_IN");

      char* contenido = malloc((tamanio_registro + 1) * sizeof(char));

      strcpy(contenido,leer_de_memoria(direccion_fisica,tamanio_registro));
      
      log_warning(logger,"ACCESO A ESPACIO DE USUARIO: PID: <%d> - Acción: <LEER> - Dirección física: <%d> - Tamaño: <%d> - Origen: <CPU>",
                          pid,
                          direccion_fisica,
                          tamanio_registro
                  );
      enviar_mensaje_a_cliente(contenido,socket_cpu);
      log_info(logger, "[MEMORIA]: MENSAJE ENVIADO A CPU: <%s> COMO MOTIVO DE FIN DE MOV_IN", contenido);
      free(contenido);
      break;

    case MOV_OUT:
      char* valor_a_escribir = malloc(sizeof(char) * (tamanio_registro + 1));
      memcpy(&valor_a_escribir, buffer->stream + sizeof(int32_t), sizeof(char) * (tamanio_registro + 1));
            buffer->stream += (tamanio_registro + 1); 

      log_info(logger, "[MEMORIA]: INSTRUCCION recibida: MOV_OUT");
      log_info(logger, "VALOR A ESCRIBIR: <%s>", valor_a_escribir);
      
      escribir_en_memoria(valor_a_escribir,direccion_fisica,tamanio_registro);

      log_warning(logger,"ACCESO A ESPACIO DE USUARIO: PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d> - Tamaño: <%d> - Origen: <CPU>",
                          pid,
                          direccion_fisica,
                          tamanio_registro
                  );
      enviar_mensaje_a_cliente("[MEMORIA]: MOV_OUT:<OK>",socket_cpu);
      log_info(logger,"MEMORIA: ENVIE EL MENSAJE <OK> A CPU COMO MOTIVO DE FIN DE MOV_OUT");
      break;
    
  default:
    log_warning(logger, "[MEMORIA]: Código Instrucción desconocido.");
    break;
  }
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

void recibir_instruccion_file_system()
{
  Lista* lista = obtener_paquete_como_lista(socket_file_system);

  int numero_op = *(int*)list_get(lista,0);

  switch (numero_op)
  {
  case F_WRITE:
    //...
    break;
  
  case F_READ:
    //....
    break;

  default:
    log_error(logger,"CODIGO DE OP DESCONOCIDO AL RECIBIR INSTRUCCION FS");
    return;
  }

}