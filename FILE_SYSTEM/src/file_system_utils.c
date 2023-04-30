#include "file_system_utils.h"

Logger *logger;
Config *config;
Hilo hilo_fileSystem;
int socket_fileSystem;
int socket_memoria;

void iniciar_logger_fileSystem()
{
    logger = log_create(ARCHIVO_LOGGER, "File_System", 1, LOG_LEVEL_INFO);
    log_info(logger, "[FILE_SYSTEM]: Logger creado correctamente");
}

void iniciar_config_fileSystem()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_fileSystem(config);
    log_info(logger,"[FILE_SYSTEM]: Archivo Config creado y rellenado correctamente");
}

void iniciar_servidor_fileSystem()
{
  log_info(logger, "[FILE_SYSTEM]: Iniciando Servidor ...");
  socket_fileSystem = iniciar_servidor(FileSystemConfig.IP, FileSystemConfig.PUERTO_ESCUCHA);

  if (socket_fileSystem < 0)
  {
    log_error(logger, "[FILE_SYSTEM]: Error intentando iniciar Servidor.");
    return EXIT_FAILURE;
  }

  log_info(logger, "[FILE_SYSTEM]: Servidor iniciado correctamente.");
}

void conectar_con_kernel()
{
    // Utiliza socket_fileSystem

    pthread_create(&hilo_fileSystem, NULL, (void *)esperar_kernel, (void *)socket_fileSystem);
    pthread_join(hilo_fileSystem, NULL);
}

void conectar_con_memoria(){

  log_info(logger, "[FILESYSTEM] conectando con Memoria...");
  socket_memoria = iniciar_servidor(FileSystemConfig.IP, FileSystemConfig.PUERTO_MEMORIA);

  if (socket_memoria < 0)
  {
      log_info(logger, "[FILESYSTEM]: Error al conectar con Memoria. Finalizando Ejecucion");
      log_error(logger, "[FILESYSTEM]: Memoria no está disponible");
      return FAILURE;
  }
  log_info(logger, "[FILESYSTEM]: Conexion con Memoria: OK");

  return SUCCESS;
}

void terminar_ejecucion(){
    log_warning(logger, "[FILE_SYSTEM]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}