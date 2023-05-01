#include "memoria_utils.h"

Logger *logger;
Config *config;
Hilo hilo;
Hilo hilo_cpu;
int socket_memoria;
int socket_kernel;

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
    // Utiliza socket_memoria

    pthread_create(&hilo, NULL, (void *)esperar_kernel, (void *)socket_memoria);
    pthread_join(hilo, NULL);
}

void conectar_con_cpu()
{
    // Utiliza socket_memoria

    pthread_create(&hilo_cpu, NULL, (void *)esperar_cpu, (void *)socket_memoria);
    pthread_join(hilo_cpu, NULL);
}

void terminar_ejecucion(){
    log_warning(logger, "[MEMORIA]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}
