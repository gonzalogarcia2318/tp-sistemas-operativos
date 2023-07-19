#include "memoria_thread.h"

t_list* tabla_test;

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
      t_list* tabla_de_segmentos = manejar_crear_proceso();
      tabla_test = tabla_de_segmentos;
      enviar_tabla_de_segmentos_a_kernel(tabla_de_segmentos);
      log_info(logger,"ENVÍE TABLA DE SEGMENTOS A KERNEL COMO MOTIVO DE FIN DE CREAR_PROCESO");
      break;
    
    case FINALIZAR_PROCESO:
      manejar_finalizar_proceso();
      break;
    
    case INSTRUCCION:
      log_info(logger, "[MEMORIA]: INSTRUCCION recibida de KERNEL");
      recibir_instruccion_kernel();
      break;

    case CONSOLIDAR:
      log_info(logger,"Recibi de Kernel: CONSOLIDAR");
      BUFFER* buffer = recibir_buffer(socket_kernel);
      int32_t un_pid;
      memcpy(&un_pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
      buffer->stream += (sizeof(int32_t)*2);
      log_info(logger,"un_pid %d", un_pid);

      compactar();
      log_info(logger,"TERMINA EL COMPACTAR");
      enviar_tabla_de_segmentos_a_kernel_despues_de_consolidar(tabla_de_segmentos_globales); //CHECKEAR
      log_info(logger,"ENVÍE TABLAS DE SEGMENTOS A KERNEL COMO MOTIVO DE FIN DE COMPACTACIÓN");
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
    
    case READ:
      log_warning(logger,"[MEMORIA]: READ recibido de FILE SYSTEM");
      
      char* leido =  manejar_read_file_system();
      log_info(logger,"[MEMORIA]: READ DE FS: %s", leido);

      PAQUETE* paquete_read = crear_paquete(READ);
      agregar_a_paquete(paquete_read, leido, strlen(leido)*sizeof(char));
      enviar_paquete_a_cliente(paquete_read, socket_fs);
      log_info(logger,"[MEMORIA]: ENVIO PAQUETE A FS: %s", leido);
      eliminar_paquete(paquete_read);
      break;

    case WRITE:
      log_warning(logger,"[MEMORIA]: WRITE recibido de FILE SYSTEM");

      manejar_write_file_system();

      PAQUETE* paquete_write = crear_paquete(WRITE);
      enviar_paquete_a_cliente(paquete_write,socket_fs);
      eliminar_paquete(paquete_write);
      break;
    
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

  log_info(logger, "//// MEMORIA  LLEGA DE CPU: TAMAÑO %d ////", tamanio_registro);

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

      char* contenido = malloc(tamanio_registro);

      strcpy(contenido,leer_de_memoria(direccion_fisica,tamanio_registro));
      
      log_warning(logger,"ACCESO A ESPACIO DE USUARIO: PID: <%d> - Acción: <LEER> - Dirección física: <%d> - Tamaño: <%d> - Origen: <CPU>",
                          pid,
                          direccion_fisica,
                          tamanio_registro
                  );
      //enviar_mensaje_a_cliente(contenido,socket_cpu);
      //log_info(logger, "[MEMORIA]: MENSAJE ENVIADO A CPU: <%s> COMO MOTIVO DE FIN DE MOV_IN", contenido);
      PAQUETE* paquete_mov_in = crear_paquete(MOV_IN);
      agregar_a_paquete(paquete_mov_in, contenido, strlen(contenido));
      enviar_paquete_a_cliente(paquete_mov_in, socket_cpu);
      log_info(logger, "[MEMORIA]: FIN MOV_IN - ENVIO: %s", contenido);
      eliminar_paquete(paquete_mov_in);
      //free(contenido);
      break;

    case MOV_OUT:
      char* valor_a_escribir = malloc(sizeof(char) * (tamanio_registro));
      memcpy(valor_a_escribir, buffer->stream + sizeof(int32_t), sizeof(char) * (tamanio_registro));
            buffer->stream += (tamanio_registro); 

      valor_a_escribir[tamanio_registro] = '\0';

      log_info(logger, "[MEMORIA]: INSTRUCCION recibida: MOV_OUT");
      log_info(logger, "VALOR A ESCRIBIR: <%s>", valor_a_escribir); //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
      
      escribir_en_memoria(valor_a_escribir,direccion_fisica,tamanio_registro);

      log_warning(logger,"ACCESO A ESPACIO DE USUARIO: PID: <%d> - Acción: <ESCRIBIR> - Dirección física: <%d> - Tamaño: <%d> - Origen: <CPU>",
                          pid,
                          direccion_fisica,
                          tamanio_registro
                  );
      //enviar_mensaje_a_cliente("[MEMORIA]: MOV_OUT:<OK>",socket_cpu);
      //log_info(logger,"MEMORIA: ENVIE EL MENSAJE <OK> A CPU COMO MOTIVO DE FIN DE MOV_OUT");
      PAQUETE* paquete_mov_out = crear_paquete(MOV_OUT);
      agregar_a_paquete(paquete_mov_out, &pid, sizeof(int32_t));
      enviar_paquete_a_cliente(paquete_mov_out, socket_cpu);
      log_info(logger,"MEMORIA: FIN DE MOV_OUT");
      eliminar_paquete(paquete_mov_out);
      
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
    case CREATE_SEGMENT: 
      
      int32_t tamanio_segmento;
      memcpy(&tamanio_segmento, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 

      log_info(logger,"INSTRUCCIÓN KERNEL: CREATE_SEGMENT - PID:<%d> - ID_SEG:<%d> - TS: <%d>", //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
                pid,
                id_segmento,
                tamanio_segmento
              );

      int base = manejar_crear_segmento(pid, id_segmento, tamanio_segmento);
      
      if (base > 0) //OK
      {
        PAQUETE * paquete_ok = crear_paquete(CREAR_SEGMENTO);
        agregar_a_paquete(paquete_ok, &base, sizeof(int32_t)); //ENVIAR_DIRE_BASE
        enviar_paquete_a_cliente(paquete_ok, socket_kernel);
        eliminar_paquete(paquete_ok);
        break;
      }
      else if (base == -2) //CONSOLIDAR (HAY ESPACIO NO CONTIGUO)
      {
        PAQUETE * paquete_consolidar = crear_paquete(CONSOLIDAR);
        agregar_a_paquete(paquete_consolidar,&pid,sizeof(int32_t));
        enviar_paquete_a_cliente(paquete_consolidar, socket_kernel);
        eliminar_paquete(paquete_consolidar);
        log_info(logger, "Enviar HAY QUE CONSOLIDAR A KERNEL");
        break;
      }
      else if (base == -3) //FALTA ESPACIO "Out of Memory"
      {
        PAQUETE * paquete_fallo = crear_paquete(FALTA_MEMORIA);
        enviar_paquete_a_cliente(paquete_fallo, socket_kernel);
        eliminar_paquete(paquete_fallo);
        break;
      }
      else // base = 0 (ERROR EN ALGUNA FUNCIÓN ANTERIOR, VERIFICAR LOGS)
      {
        log_error(logger,"ERROR AL ASIGNAR BASE");
        break;
      }
    break;

    case DELETE_SEGMENT:
  
      log_info(logger,"INSTRUCCIÓN KERNEL: DELETE_SEGMENT - PID:<%d> - ID_SEG:<%d>", //PARA COMPROBAR QUE LLEGA BIEN, ELIMINAR
                pid,
                id_segmento
              );

      t_list* tabla_de_segmentos = obtener_tabla_de_segmentos(pid);
      SEGMENTO* segmento = obtener_segmento_de_tabla_de_segmentos(tabla_de_segmentos,id_segmento);

      if(segmento == NULL)
      {
        log_error(logger,"ERROR AL RECUPERAR SEGMENTO DE TABLA DE SEGMENTOS DEL PROCESO");
        break;
      }

      manejar_eliminar_segmento(segmento);

      log_warning(logger,"PID: <%d> - Eliminar Segmento: ID SEGMENTO: <%d> - BASE: <%d> - TAMAÑO: <%d>",
                          pid,
                          id_segmento,
                          segmento->base,
                          segmento->limite
                  );

      enviar_tabla_de_segmentos_a_kernel_por_delete_segment(tabla_de_segmentos);

      break;

    default:
      log_warning(logger, "[MEMORIA]: Código Instrucción desconocido.");
      break;
  }
}
