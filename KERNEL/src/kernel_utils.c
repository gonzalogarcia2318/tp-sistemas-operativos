#include <kernel_utils.h>

void iniciar_logger_kernel()
{
    logger = log_create(ARCHIVO_LOGGER, "Kernel", 1, LOG_LEVEL_INFO);
    log_info(logger, "[KERNEL]: Logger creado correctamente");
}

void iniciar_config_kernel()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_kernel(config);
    log_info(logger,"[KERNEL]: Archivo Config creado y rellenado correctamente");
}

void iniciar_servidor_kernel()
{
  log_info(logger, "[KERNEL]: Iniciando Servidor ...");
  socket_kernel = iniciar_servidor(KernelConfig.IP, KernelConfig.PUERTO_ESCUCHA);

  if (socket_kernel < 0)
  {
    log_error(logger, "[KERNEL]: Error intentando iniciar Servidor.");
    return EXIT_FAILURE;
  }

  log_info(logger, "[KERNEL]: Servidor iniciado correctamente.");
}

void conectar_con_consola()
{
    // Utiliza socket_kernel

    pthread_create(&hilo_consolas, NULL, (void *)esperar_consola, (void *)socket_kernel);
    pthread_join(hilo_consolas, NULL);
}

void terminar_ejecucion(){
    log_warning(logger, "[KERNEL]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}