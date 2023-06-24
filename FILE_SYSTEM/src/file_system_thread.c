#include "file_system_thread.h"

extern t_bitarray *bitmap ;
extern FILE* archivo_bloques;

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

  t_list* lista =  obtener_paquete_estructura_dinamica(socket_kernel);

  CODIGO_INSTRUCCION cod_instruccion = *(CODIGO_INSTRUCCION*)list_get(lista,0);
  char* nombre_archivo = (char*)list_get(lista,1); //Verificar posicion de la instruccion
  int32_t direccion_fisica = 0;
  int32_t tamanio = 0;
  int32_t puntero_archivo = 0; //DEFINIR DE DONDE SALE ESTE DATO

  switch (cod_instruccion)
  {
    case CREAR_ARCHIVO:
      log_warning(logger,"CREAR ARCHIVO: <NOMBRE_ARCHIVO: %s>", nombre_archivo);
      if(crear_archivo(nombre_archivo)!=-1){
        log_warning(logger,"FCB CREADO DE: %s>", nombre_archivo);

        enviar_mensaje_a_cliente("CREACION_OK", socket_kernel);
      }
      break;
    case F_OPEN:
      log_warning(logger,"ABRIR ARCHIVO: <NOMBRE_ARCHIVO: %s>", nombre_archivo);
      if(existe_archivo(nombre_archivo) == SUCCESS){
        enviar_mensaje_a_cliente("OK", socket_kernel);
      }
      else
      {
        enviar_mensaje_a_cliente("No Existe Archivo", socket_kernel);
      }
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
      tamanio = *(int32_t*)list_get(lista,1);
      log_warning(logger, "TRUNCAR ARCHIVO: <NOMBRE_ARCHIVO: %s> - Tamaño: <TAMAÑO: %d>",
                          nombre_archivo,
                          tamanio);

      ejecutar_f_truncate(nombre_archivo,tamanio); 

      enviar_mensaje_a_cliente("F_TRUNCATE: OK", socket_kernel);
      break;

    default:
      log_error(logger,"FILE SYSTEM: ERROR: COD_INSTRUCCION DESCONOCIDO");
      break;
  }

  list_destroy(lista);
}

 // crea un archivo FCB correspondiente al nuevo archivo, con tamaño 0 y sin bloques asociados.
int crear_archivo(char* nombre){
 

// Crea el directorio
  mkdir(FileSystemConfig.PATH_FCB,0777);
  char* pathCompleto = FileSystemConfig.PATH_FCB;
  string_append(&pathCompleto,nombre);
  string_append(&pathCompleto,".config");

  t_config fcb_config;
  fcb_config.path = pathCompleto;

  t_dictionary * diccionario = dictionary_create();
  dictionary_put(diccionario,"NOMBRE_ARCHIVO", nombre);
  dictionary_put(diccionario,"TAMANIO_ARCHIVO", "0");
  dictionary_put(diccionario,"PUNTERO_DIRECTO", "0");
  dictionary_put(diccionario,"PUNTERO_INDIRECTO", "0");
  fcb_config.properties = diccionario;


  return config_save_in_file(&fcb_config, pathCompleto);

}

int existe_archivo(char* nombre){

  char* path = "config/fcb/Prueba.config"; //Hacer el path correcto, revisar porque falla
  // string_append(&path,nombre);
  // string_append(&path,".config");
  t_config *fcb=  config_create(path);

  if(fcb == NULL){
    return FAILURE;
  }
  else
  {
    return SUCCESS;
  } 
}

void ejecutar_f_truncate(char* nombre_archivo,int a_truncar){
   char* path = "config/fcb/Prueba.config"; //Hacer el path correcto, revisar porque falla
  // string_append(&path,nombre);
  // string_append(&path,".config");
   t_config *fcb=  config_create(path);
   int tamanio = config_get_int_value(fcb, "TAMANIO_ARCHIVO");
   int puntero_directo = config_get_int_value(fcb, "PUNTERO_DIRECTO");
   int puntero_indirecto = config_get_int_value(fcb, "PUNTERO_INDIRECTO");
   
  if(a_truncar>= tamanio){
    //ampliar tamañano
    //buscar bloques libres recorriendo el bitmap
    //una vez conseguido los bloques, asignar los punteros de acuerdo a la cantidad de bloques que sea necesario(bytes)
    //actualizar el tamaño del archivo en FCB
  }
  else
  {
    //reducir tamanio
    //marcar como libres ls bloques en el bitmap
    //descartar los bloques , actualizando los valores de los punteros(descartando desde el final del archivo hacia el principio)
    //actualizar el tamaño del archivo en FCB
  }
  

}