#include "memoria_utils.h"

Logger *logger;
Config *config;
Hilo hilo_kernel;
Hilo hilo_cpu;
Hilo hilo_file_system;
int socket_memoria;
int socket_kernel;
int socket_file_system;
int socket_cpu;

void iniciar_logger_memoria()
{
    logger = log_create(ARCHIVO_LOGGER, "Memoria", 1, LOG_LEVEL_INFO);
    log_info(logger, "[MEMORIA]: Logger creado correctamente");
}

void iniciar_config_memoria()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_memoria(config);
    log_info(logger,"[MEMORIA]: Archivo Config creado y rellenado correctamente");
}

int iniciar_servidor_memoria()
{
  log_info(logger, "[MEMORIA]: Iniciando Servidor ...");
  socket_memoria = iniciar_servidor(MemoriaConfig.IP, MemoriaConfig.PUERTO_ESCUCHA);

  if (socket_memoria < 0)
  {
    log_error(logger, "[MEMORIA]: Error intentando iniciar Servidor.");
    return FAILURE;
  }

  log_info(logger, "[MEMORIA]: Servidor iniciado correctamente.");
  return SUCCESS;
}

void conectar_con_kernel()
{
    log_info(logger, "[MEMORIA]: Esperando conexiones de Kernel...");
    socket_kernel = esperar_cliente(socket_memoria);
    log_info(logger, "[MEMORIA]: Conexión de Kernel establecida.");

    pthread_create(&hilo_kernel, NULL, (void *)escuchar_kernel, (void *)socket_kernel);
}

void conectar_con_file_system()
{
    log_info(logger, "[MEMORIA]: Esperando conexiones de FILE SYSTEM...");
    socket_file_system = esperar_cliente(socket_memoria);
    log_info(logger, "[MEMORIA]: Conexión de FILE SYSTEM establecida.");

    pthread_create(&hilo_file_system, NULL, (void *)escuchar_file_system, (void *)socket_file_system);
   
}

void conectar_con_cpu()
{
    log_info(logger, "[MEMORIA]: Esperando conexiones de CPU...");
    socket_cpu = esperar_cliente(socket_memoria);
    log_info(logger, "[MEMORIA]: Conexión de CPU establecida.");
    pthread_create(&hilo_cpu, NULL, (void *)escuchar_cpu, (void *)socket_cpu);
    
}

void terminar_ejecucion(){
    log_warning(logger, "[MEMORIA]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}
