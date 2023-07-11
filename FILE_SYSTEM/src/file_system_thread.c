#include "file_system_thread.h"

extern t_bitarray *bitmap ;
extern SUPERBLOQUE superbloque;
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
  int32_t puntero_archivo = 0;

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
      direccion_fisica = *(int32_t*) list_get(lista, 4); 
      tamanio = *(int32_t*)list_get(lista,3); 
      puntero_archivo = *(int32_t*) list_get(lista, 2); 
      log_warning(logger,"LEER ARCHIVO: <NOMBRE_ARCHIVO: %s> - <PUNTERO ARCHIVO: %d> - <DIRECCION MEMORIA: %d>> - <TAMAÑO: %d>",
                          nombre_archivo,
                          puntero_archivo,
                          direccion_fisica,
                          tamanio);
                          
      if(ejecutar_f_read(nombre_archivo,puntero_archivo,tamanio,direccion_fisica) == SUCCESS){
          enviar_mensaje_a_cliente("F_READ: OK", socket_kernel);
      }
      else
      {
        enviar_mensaje_a_cliente("F_READ: ERROR", socket_kernel);
      }
      
      break;
        
    case F_WRITE:
      direccion_fisica = *(int32_t*) list_get(lista, 4); 
      tamanio = *(int32_t*)list_get(lista,3); 
      puntero_archivo =  *(int32_t*) list_get(lista, 2); 
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
   //mejorar
  Config * config = config_create("config/file_system.config");
  char* pathCompleto = config_get_string_value(config, "PATH_FCB");

// Crea el directorio
  mkdir(pathCompleto,0777);

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
  //mejorar
  Config * config = config_create("config/file_system.config");
  char* pathCompleto = config_get_string_value(config, "PATH_FCB");
  string_append(&pathCompleto,nombre);
  string_append(&pathCompleto,".config");
  t_config *fcb=  config_create(pathCompleto);

  if(fcb == NULL){
    return FAILURE;
  }
  else
  {
    return SUCCESS;
  } 
}

void ejecutar_f_truncate(char* nombre,int a_truncar){
  //mejorar
  Config * config = config_create("config/file_system.config");
  char* pathCompleto = config_get_string_value(config, "PATH_FCB");
  string_append(&pathCompleto,nombre);
  string_append(&pathCompleto,".config");
  // string_append(&path,nombre);
  // string_append(&path,".config");
   t_config *fcb=  config_create(pathCompleto);
   int tamanio = config_get_int_value(fcb, "TAMANIO_ARCHIVO");
   uint32_t puntero_directo = config_get_int_value(fcb, "PUNTERO_DIRECTO");
   uint32_t puntero_indirecto = config_get_int_value(fcb, "PUNTERO_INDIRECTO");
  //maximoValor puntero
  int maxPuntero = 99; // calcularlo
  // buffers
  char puntero_directo_str[maxPuntero];  
  char puntero_indirecto_str[maxPuntero]; 
  char tamanio_archivo_str[maxPuntero];   
   
  if(a_truncar>= tamanio){
    //calcular bloques necesarios
    int bloques_necesarios = ceil((double)(a_truncar - tamanio) / superbloque.BLOCK_SIZE); 
    int bloques_minimos = (bloques_necesarios >=2) ? 2 : 1;
  
    //buscar bloques libres
    puntero_directo = buscar_bloque_libre();
    // Convertir uint32_t a string
    snprintf(puntero_directo_str, sizeof(puntero_directo_str), "%u", puntero_directo);
    config_set_value(fcb,"PUNTERO_DIRECTO", puntero_directo_str); 

      if(bloques_necesarios > 1){
        puntero_indirecto = buscar_bloque_libre();

        archivo_bloques= fopen(FileSystemConfig.PATH_BLOQUES,"wb+");

        for(int bloques_restantes = bloques_necesarios-1;bloques_restantes>0;bloques_restantes--){
          fseek(archivo_bloques,puntero_indirecto,SEEK_SET);
          
          uint32_t bloque_sgt = buscar_bloque_libre();
          aplicar_retardo_acceso_bloque();
          log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                          nombre,
                          puntero_indirecto/superbloque.BLOCK_SIZE,
                          bloque_sgt/superbloque.BLOCK_SIZE);

          fwrite(&bloque_sgt,sizeof(uint32_t),1,archivo_bloques);
        }
        fclose(archivo_bloques);
        snprintf(puntero_indirecto_str, sizeof(puntero_indirecto_str), "%u", puntero_indirecto);
        config_set_value(fcb,"PUNTERO_INDIRECTO", puntero_indirecto_str);
      }
    //actualizar el tamaño del archivo en FCB
    snprintf(tamanio_archivo_str, sizeof(tamanio_archivo_str), "%u", tamanio + a_truncar);
    config_set_value(fcb,"TAMANIO_ARCHIVO", tamanio_archivo_str);

  }
  else  //reducir tamanio
  {
    int a_reducir = tamanio - a_truncar;
    int cant_ptrs = (tamanio/superbloque.BLOCK_SIZE) -1; //se restan  por el ptr directo
    int bloques_restantes = ceil((double)(a_reducir) / superbloque.BLOCK_SIZE);

    FILE *bm = fopen("config/bitmap.dat", "rb+"); //quitar hardcod
    fread(bitmap, sizeof(bitmap->size), 1, bm);

    if(bloques_restantes>1)
      {
        archivo_bloques= fopen(FileSystemConfig.PATH_BLOQUES,"rb+");
        int copybr= bloques_restantes;
        for(int i = 1;i<=bloques_restantes;i++){
              uint32_t valor_puntero;
              uint32_t pos_puntero = puntero_indirecto + (sizeof(uint32_t) * copybr);

              fseek(archivo_bloques,pos_puntero,SEEK_SET);
              fread(&valor_puntero, sizeof(uint32_t), 1, archivo_bloques);
              //marcar como libres en el bitmap
              bitarray_clean_bit(bitmap, valor_puntero/superbloque.BLOCK_SIZE);
              copybr--;
            //se dejan los valores en el archivo de bloques, pero al marcarse libres, otro proceso pisara los valores
            } 
        fclose(archivo_bloques);
      }
    else{
      archivo_bloques= fopen(FileSystemConfig.PATH_BLOQUES,"rb+");
        for(int j = 1;j<=cant_ptrs;j++){
              uint32_t valor_puntero;
              uint32_t pos_puntero = puntero_indirecto + (sizeof(uint32_t) * cant_ptrs);

              fseek(archivo_bloques,pos_puntero,SEEK_SET);
              fread(&valor_puntero, sizeof(uint32_t), 1, archivo_bloques);
              aplicar_retardo_acceso_bloque();
              log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                          nombre,
                          puntero_indirecto/superbloque.BLOCK_SIZE,
                          valor_puntero/superbloque.BLOCK_SIZE);
              //marcar como libres en el bitmap
              bitarray_clean_bit(bitmap, valor_puntero/superbloque.BLOCK_SIZE);
            //se dejan los valores en el archivo de bloques, pero al marcarse libres, otro proceso pisara los valores
            } 
        fclose(archivo_bloques);
    }
    fclose(bm);
    //actualizar el tamaño del archivo en FCB
    snprintf(tamanio_archivo_str, sizeof(tamanio_archivo_str), "%u", a_truncar);
    config_set_value(fcb,"TAMANIO_ARCHIVO", tamanio_archivo_str);
  }
  config_save(fcb);
}

int buscar_bloque_libre(){
  int ptr = 0;
  off_t index;
  char* path = "config/bitmap.dat"; //quitar hardcod
  FILE *file = fopen(path, "rb+");

  fread(bitmap, sizeof(bitmap->size), 1, file);


  for(index= 0;index<bitmap->size;index++){
    bool estado = bitarray_test_bit(bitmap, index);
    log_warning(logger, "ACCESO A BITMAP: <NUMERO BLOQUE: %d> - Estado: <ESTADO>: %d>",
                          index,
                          estado);
    if(!estado){
      break;
    }
  }
  ptr = index * superbloque.BLOCK_SIZE;

  bitarray_set_bit(bitmap, index);
  fclose(file);
  return ptr; 
}

int ejecutar_f_read(nombre_archivo,puntero_archivo,tamanio, direccion_fisica){
  //abrir archivo de bloques
  archivo_bloques= fopen(FileSystemConfig.PATH_BLOQUES,"rb+");
  char* valor_leido;

  fseek(archivo_bloques,puntero_archivo,SEEK_SET);
  fread(&valor_leido,sizeof(tamanio),1,archivo_bloques);

  aplicar_retardo_acceso_bloque();
  log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                          nombre_archivo,
                          puntero_archivo/superbloque.BLOCK_SIZE, //hay que hacer el mapeo con el numero de bloque de archivo
                          puntero_archivo/superbloque.BLOCK_SIZE); 

  log_warning(logger, "VALOR LEIDO: Archivo: <NOMBRE_ARCHIVO>: %s",
                          valor_leido);
  //enviar a memoria(direccion_fisica, valor_leido)

 //int mensaje = esperar_mensaje_memoria()

 fclose(archivo_bloques);

 //return mensaje;
}

int ejecutar_f_write(nombre_archivo,puntero_archivo,tamanio, direccion_fisica){

  archivo_bloques= fopen(FileSystemConfig.PATH_BLOQUES,"wb+");
  char *datos;
  //datos= obtener info de memoria(direccion_fisica)

  fseek(archivo_bloques,puntero_archivo,SEEK_SET);
  fwrite(&datos,sizeof(tamanio),1,archivo_bloques);

  log_warning(logger, "VALOR ESCRITO: Archivo: <DATOS>: %s",
                          datos);

  fclose(archivo_bloques);
 //return ok;
}