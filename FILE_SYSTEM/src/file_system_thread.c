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
    
    case INSTRUCCION:
      log_info(logger, "[FILE SYSTEM]: INSTRUCCION recibida de KERNEL");
      recibir_instruccion_kernel();
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

void recibir_instruccion_kernel()
{
  Lista* lista = obtener_paquete_como_lista(socket_kernel);
  CODIGO_INSTRUCCION cod_instruccion = *(CODIGO_INSTRUCCION*)list_get(lista,0);
  char* nombre_archivo = (char*)list_get(lista,1);
  int32_t direccion_fisica = 0;
  int32_t tamanio = 0;
  int32_t puntero_archivo = 0; //DEFINIR DE DONDE SALE ESTE DATO

  switch (cod_instruccion)
  {
    case F_OPEN:
      log_warning(logger,"CREAR ARCHIVO: <NOMBRE_ARCHIVO: %s>", nombre_archivo);

      //ejecutar_f_open(nombre_archivo); TODO

      enviar_mensaje_a_cliente("F_OPEN: OK", socket_kernel);
      break;
        
    case F_CLOSE:
      log_warning(logger,"CERRAR ARCHIVO: <NOMBRE_ARCHIVO: %s>", nombre_archivo);

      //ejecutar_f_close(nombre_archivo); TODO

      enviar_mensaje_a_cliente("F_CLOSE: OK", socket_kernel);
      break;
        
    case F_SEEK:
      int32_t posicion = *(int32_t*)list_get(lista,2); 
      log_warning(logger,"ACTUALIZAR PUNTERO DE ARCHIVO: <NOMBRE_ARCHIVO: %s - POSICION: %d>",
                            nombre_archivo,
                            posicion
                  );

      //ejecutar_f_seek(nombre_archivo,posicion); TODO

      enviar_mensaje_a_cliente("F_SEEK: OK", socket_kernel);
      break;
        
    case F_READ:
      direccion_fisica = *(int32_t*) list_get(lista, 2); 
      tamanio = *(int32_t*)list_get(lista,3); 
      puntero_archivo = 0; //DEFINIR DE DONDE SALE ESTE DATO
      log_warning(logger,"LEER ARCHIVO: <NOMBRE_ARCHIVO: %s> - <PUNTERO ARCHIVO: %d> - <DIRECCION MEMORIA: %d>> - <TAMAÑO: %d>",
                          nombre_archivo,
                          puntero_archivo,
                          direccion_fisica,
                          tamanio);
                          
      //ejecutar_f_read(nombre_archivo,puntero_archivo,direccion_fisica,tamanio); TODO

      enviar_mensaje_a_cliente("F_READ: OK", socket_kernel);
      break;
        
    case F_WRITE:
      direccion_fisica = *(int32_t*) list_get(lista, 2); 
      tamanio = *(int32_t*)list_get(lista,3); 
      puntero_archivo = 0; //DEFINIR DE DONDE SALE ESTE DATO
      log_warning(logger,"ESCRIBIR ARCHIVO: <NOMBRE_ARCHIVO: %s> - <PUNTERO ARCHIVO: %d> - <DIRECCION MEMORIA: %d>> - <TAMAÑO: %d>",
                          nombre_archivo,
                          puntero_archivo,
                          direccion_fisica,
                          tamanio);

      //ejecutar_f_write(nombre_archivo,puntero_archivo,direccion_fisica,tamanio); TODO

      enviar_mensaje_a_cliente("F_WRITE: OK", socket_kernel);
      break;
        
    case F_TRUNCATE:
      tamanio = *(int32_t*)list_get(lista,2);
      log_warning(logger, "TRUNCAR ARCHIVO: <NOMBRE_ARCHIVO: %s> - Tamaño: <TAMAÑO: %d>",
                          nombre_archivo,
                          tamanio);

      //ejecutar_f_truncate(nombre_archivo,tamanio); TODO

      enviar_mensaje_a_cliente("F_TRUNCATE: OK", socket_kernel);
      break;

    default:
      log_error(logger,"FILE SYSTEM: ERROR: COD_INSTRUCCION DESCONOCIDO");
      break;
  }

  list_destroy(lista);
}
