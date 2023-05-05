#include "file_system_utils.h"

Logger *logger;
Config *config;
Hilo hilo_fileSystem;
int socket_file_system;
int socket_memoria;
int socket_kernel;

void iniciar_logger_file_system()
{
    logger = log_create(ARCHIVO_LOGGER, "File_System", 1, LOG_LEVEL_INFO);
    log_info(logger, "[FILE_SYSTEM]: Logger creado correctamente");
}

void iniciar_config_file_system()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_file_system(config);
    log_info(logger,"[FILE_SYSTEM]: Archivo Config creado y rellenado correctamente");
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
 log_info(logger, "[MEMORIA]: Esperando conexiones de Kernel...");
  socket_kernel = esperar_cliente(socket_file_system);
  log_info(logger, "[MEMORIA]: Conexión de Kernel establecida.");

  pthread_create(&hilo_fileSystem, NULL, (void *)manejar_paquete_kernel, (void *)socket_kernel);
  pthread_join(hilo_fileSystem, NULL);
}

void conectar_con_memoria(){

  log_info(logger, "[FILESYSTEM] conectando con Memoria...");
  socket_memoria = crear_conexion_con_servidor(FileSystemConfig.IP_MEMORIA, FileSystemConfig.PUERTO_MEMORIA);

  if (socket_memoria < 0)
  {
    log_info(logger, "[FILESYSTEM]: Error al conectar con Memoria. Finalizando Ejecucion");
    log_error(logger, "[FILESYSTEM]: Memoria no está disponible");
    return FAILURE;
  }
  log_info(logger, "[FILESYSTEM]: Conexion con Memoria: OK");
  enviar_mensaje_a_servidor("HOLA! SOY FILE SYSTEM (●'◡'●)",socket_memoria);
}

void terminar_ejecucion(){
    log_warning(logger, "[FILE_SYSTEM]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}
