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
      int pid;
      t_list* tabla_de_segmentos = manejar_crear_proceso(&pid);
      enviar_tabla_de_segmentos_a_kernel(tabla_de_segmentos, pid);
      log_info(logger,"ENVÍE TABLA DE SEGMENTOS A KERNEL COMO MOTIVO DE FIN DE CREAR_PROCESO");
      break;
    
    case FINALIZAR_PROCESO:
      manejar_finalizar_proceso();
      enviar_mensaje_a_cliente("FINALIZAR_PROCESO: <OK>",socket_kernel);
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

  int32_t cod_instruccion;
  int32_t pid;
  int32_t direccion_fisica;
  int32_t tamanio_registro;
  
  memcpy(&cod_instruccion, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
  memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
  memcpy(&direccion_fisica, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
  memcpy(&tamanio_registro, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

  log_info(logger,"INSTRUCCIÓN CPU: COD:<%d> - PID:<%d> - DF:<%d> - TR: <%d>", //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
            cod_instruccion,
            pid,
            direccion_fisica,
            tamanio_registro
          );

  switch (cod_instruccion)
  {
    case MOV_IN:
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
      log_info(logger, "VALOR A ESCRIBIR: <%s>", valor_a_escribir); //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
      
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
  BUFFER* buffer = recibir_buffer(socket_kernel);
  int32_t cod_instruccion;
  int32_t pid;
  int32_t id_segmento;

  memcpy(&cod_instruccion, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
  memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2); 
  memcpy(&id_segmento, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
  
  switch (cod_instruccion)
  {
    case CREATE_SEGMENT: //TODO
      
      int32_t tamanio_segmento;
      memcpy(&tamanio_segmento, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
      log_info(logger,"INSTRUCCIÓN KERNEL: CREATE_SEGMENT - PID:<%d> - ID_SEG:<%d> - TS: <%d>", //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
                pid,
                id_segmento,
                tamanio_segmento
              );

      int codigo = manejar_crear_segmento(pid, id_segmento, tamanio_segmento); //TODO

      switch (codigo)
      {
      case 1: //OK
        //ENVIAR_DIRE_BASE
        break;
          
      case 2: //CONSOLIDAR (HAY ESPACIO NO CONTIGUO)
        enviar_mensaje_a_cliente("HAY QUE CONSOLIDAR", socket_kernel); //REALMENTE SERÍA PAQUETE CON COD_OP : CONSOLIDAR
        break;
          
      case 3: //FALTA ESPACIO "Out of Memory"
        enviar_mensaje_a_cliente("FALTA ESPACIO", socket_kernel);
        break;

      default:
        log_error(logger, "ERROR EN FUNCIÓN: manejar_create_segment(2)");
        break;
      }

      break;

    case DELETE_SEGMENT: //TODO
  
      log_info(logger,"INSTRUCCIÓN KERNEL: DELETE_SEGMENT - PID:<%d> - ID_SEG:<%d>", //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
                pid,
                id_segmento
              );
      //int dire_base = manejar_delete_segment(pid, id_segmento); TODO

      enviar_mensaje_a_cliente("DELETE_SEGMENT: <OK>",socket_kernel);
      log_warning(logger,"PID: <PID> - Eliminar Segmento: <ID SEGMENTO> - Base: <DIRECCIÓN BASE> - TAMAÑO: <TAMAÑO>");
     
      break;

    default:
      log_warning(logger, "[MEMORIA]: Código Instrucción desconocido.");
      break;
  }
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
