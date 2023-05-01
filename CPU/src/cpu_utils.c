#include "cpu_utils.h"

void iniciar_logger_cpu()
{
    logger = log_create(ARCHIVO_LOGGER, "CPU", 1, LOG_LEVEL_INFO);
    log_info(logger, "[CPU]: Logger creado correctamente");
}

void iniciar_config_cpu()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_cpu(config);
    log_info(logger,"[CPU]: Archivo Config creado y rellenado correctamente");
}

int iniciar_servidor_cpu()
{
  log_info(logger, "[CPU]: Iniciando Servidor ...");
  socket_cpu = iniciar_servidor(CPUConfig.IP, CPUConfig.PUERTO_ESCUCHA);

  if (socket_kernel < 0)
  {
    log_error(logger, "[CPU]: Error intentando iniciar Servidor.");
    return FAILURE;
  }

  log_info(logger, "[CPU]: Servidor iniciado correctamente.");
  return SUCCESS;
}

// Conectar con cliente de cpu? - esperar kernel
void conectar_con_kernel()
{
    // Utiliza socket_kernel

    pthread_create(&hilo_kernel, NULL, (void *)esperar_kernel, (void *)socket_cpu);
    pthread_join(hilo_kernel, NULL);
}

int conectar_con_memoria()
{
	 log_info(logger, "[CPU] conectando con memoria...");

	    socket_memoria = crear_conexion_con_servidor(CPUConfig.IP, CPUConfig.PUERTO_ESCUCHA);

	    if(socket_memoria < 0)
	    {
	        log_info(logger,"[CPU]: Error al conectar con memoria. Finalizando Ejecucion");
	        log_error(logger,"[CPU]: memoria no está disponible");

	        return FAILURE;

	    }
	    log_info(logger, "[CPU]: Conexion con Memoria: OK");

	    return SUCCESS;
}

void terminar_ejecucion(){
    log_warning(logger, "[CPU]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}
