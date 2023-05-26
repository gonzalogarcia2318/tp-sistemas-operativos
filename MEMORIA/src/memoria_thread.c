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
  Lista* lista;
  PAQUETE* paquete = crear_paquete(INSTRUCCION);

  lista = obtener_paquete_como_lista(socket_cpu);

  CODIGO_INSTRUCCION cod_instruccion = *(CODIGO_INSTRUCCION *)list_get(lista,0);

  switch (cod_instruccion)
  {
  case MOV_IN:
    log_info(logger, "[MEMORIA]: INSTRUCCION recibida: MOV_IN");
    int direccion_fisica = *(int*)list_get(lista,1);
    //...
    //leer de memoria y devolver valor de la dire_fisica dada
    //...TODO
    agregar_a_paquete(paquete,"VALOR_LEIDO",sizeof(char*));
    enviar_paquete_a_cliente(paquete,socket_cpu);
    log_info(logger, "[MEMORIA]: PAQUETE ENVIADO A CPU por instrucción recibida: MOV_IN");
    break;
  default:
    log_warning(logger, "[MEMORIA]: Código Instrucción desconocido.");
    break;
  }
  list_destroy(lista);
  eliminar_paquete(paquete);
}