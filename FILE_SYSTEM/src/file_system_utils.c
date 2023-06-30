#include "file_system_utils.h"

Logger *logger;
Config *config;
Config *config_superbloque;
Hilo hilo_fileSystem;
SUPERBLOQUE superbloque;
t_bitarray *bitmap ;
FILE* archivo_bloques;
char* bitarray;
int socket_file_system;
int socket_memoria;
int socket_kernel;

int iniciar_logger_file_system()
{
    logger = log_create(ARCHIVO_LOGGER, "File_System", 1, LOG_LEVEL_INFO);
    if(logger == NULL)
    {
      log_error(logger,"[FILE_SYSTEM]: ERROR AL LOGGER INICIAL");
      return FAILURE;
    } 
    log_info(logger, "[FILE_SYSTEM]: Logger creado correctamente");
    return SUCCESS;
}

int iniciar_config_file_system()
{
    config = config_create(ARCHIVO_CONFIG);
    if(config == NULL)
    {
      log_error(logger,"[FILE_SYSTEM]: ERROR AL INICIAR CONFIG INICIAL");
      return FAILURE;
    } 
    rellenar_configuracion_file_system(config);
    log_info(logger,"[FILE_SYSTEM]: Archivo Config creado y rellenado correctamente");
    return SUCCESS;
}

int iniciar_config_superbloque()
{
  config_superbloque = config_create(FileSystemConfig.PATH_SUPERBLOQUE);
  if(config_superbloque == NULL)
  {
    log_error(logger,"[FILE_SYSTEM]: ERROR AL INICIAR CONFIG SUPERBLOQUE");
    return FAILURE;
  } 
  rellenar_configuracion_superbloque(config_superbloque);
  log_info(logger, "[FILE_SYSTEM]: Archivo ConfigSuperbloque creado y rellenado correctamente");
  return SUCCESS;
}

void rellenar_configuracion_superbloque(Config* config_sb)
{
  superbloque.BLOCK_SIZE = config_get_int_value(config_sb,"BLOCK_SIZE");
  superbloque.BLOCK_COUNT = config_get_int_value(config_sb,"BLOCK_COUNT");
  log_info(logger,"SUPERBLOQUE: BLOCK_SIZE:<%d>, BLOCK_COUNT:<%d>",
                  superbloque.BLOCK_SIZE,
                  superbloque.BLOCK_COUNT
          );
}

int levantar_bitmap(char *path)
{
    FILE *file;
    size_t size = superbloque.BLOCK_COUNT/8; // porque el create se hace en bytes
    bitarray = malloc(size);
    file = fopen(path, "rb+");
    if (file == NULL) {
        // Si el archivo no existe, se crea
        file = fopen(path, "wb+");
        if (file == NULL) {
           log_error(logger,"Error al crear el archivo BITMAP");
            return FAILURE;
        }

         bitmap = bitarray_create_with_mode(bitarray, size, LSB_FIRST);
         fwrite(bitmap, sizeof(t_bitarray), 1, file);
         log_info(logger,"[FILE_SYSTEM]: Archivo Bitmap CREADO correctamente"); 
    }

    fread(&bitmap, sizeof(t_bitarray), 1, file); //ARREGLAR
    fclose(file);

    log_info(logger,"[FILE_SYSTEM]: Archivo Bitmap levantado correctamente");
    return SUCCESS;
}

int iniciar_archivo_de_bloques(char* path_ab)
{
  int tamanio_ab = superbloque.BLOCK_COUNT * superbloque.BLOCK_SIZE;
 
  archivo_bloques = fopen(path_ab,"r+");
  if(archivo_bloques == NULL) //NO EXISTÍA
  {
    archivo_bloques = fopen(path_ab,"w+");
      if (archivo_bloques == NULL)
      {
        log_error(logger,"[FILE_SYSTEM]: ERROR AL CREAR ARCHIVO DE BLOQUES EN BLANCO");
        return FAILURE;
      }

    int fd = fileno(archivo_bloques); // Obtiene el descriptor de archivo correspondiente al puntero FILE*

      if(ftruncate(fd,tamanio_ab) == -1)
      {
        fclose(archivo_bloques);
        log_error(logger,"[FILE_SYSTEM]: ERROR AL TRUNCAR EL ARCHIVO DE BLOQUES");
        return FAILURE;
      }

    fclose(archivo_bloques);
    log_info(logger, "[FILE_SYSTEM]: ARCHIVO DE BLOQUES CREADO y TRUNCADO DESDE 0 CORRECTAMENTE");
    return SUCCESS;
  }
  fclose(archivo_bloques);
  log_info(logger, "[FILE_SYSTEM]: ARCHIVO DE BLOQUES YA EXISTENTE; SE HA LEVANTADO CORRECTAMENTE");
  return SUCCESS;
}

int iniciar_servidor_file_system()
{
  log_info(logger, "[FILE_SYSTEM]: Iniciando Servidor ...");
  socket_file_system = iniciar_servidor(FileSystemConfig.IP, FileSystemConfig.PUERTO_ESCUCHA);

  if (socket_file_system < 0)
  {
    log_error(logger, "[FILE_SYSTEM]: Error intentando iniciar Servidor.");
    return FAILURE;
  }

  log_info(logger, "[FILE_SYSTEM]: Servidor iniciado correctamente.");
  return SUCCESS;
}

void conectar_con_kernel()
{
  log_info(logger, "[FILE_SYSTEM]: Esperando conexiones de Kernel...");
  socket_kernel = esperar_cliente(socket_file_system);
  log_info(logger, "[FILE_SYSTEM]: Conexión de Kernel establecida.");

  pthread_create(&hilo_fileSystem, NULL, (void *)manejar_paquete_kernel, (void *)socket_kernel);
  pthread_join(hilo_fileSystem, NULL);
}

int conectar_con_memoria(){

  log_info(logger, "[FILESYSTEM] conectando con Memoria...");
  socket_memoria = crear_conexion_con_servidor(FileSystemConfig.IP_MEMORIA, FileSystemConfig.PUERTO_MEMORIA);

  if (socket_memoria < 0)
  {
    log_error(logger, "[FILESYSTEM]: Error al conectar con Memoria. Finalizando Ejecucion");
    log_error(logger, "[FILESYSTEM]: Memoria no está disponible");
    return FAILURE;
  }
  log_info(logger, "[FILESYSTEM]: Conexion con Memoria: OK");
  enviar_mensaje_a_servidor("HOLA! SOY FILE SYSTEM (●'◡'●)",socket_memoria);
  return SUCCESS;
}

void terminar_ejecucion(){
    log_warning(logger, "[FILE_SYSTEM]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}
