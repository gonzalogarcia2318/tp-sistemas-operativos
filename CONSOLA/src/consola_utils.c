#include "consola_utils.h"

Logger *logger;
Config *config;
int socket_kernel;

void test1()
{
    printf("test");
}

void inicializar_logger_consola()
{
    logger = log_create(ARCHIVO_LOGGER, "Consola", true, LOG_LEVEL_INFO);
    log_info(logger, "[CONSOLA]: Logger creado correctamente");
}

void inicializar_config_consola()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_consola(config);
    log_info(logger, "[CONSOLA]: Archivo Config creado y rellenado correctamente");
}

int conectar_con_kernel()
{
    log_info(logger, "[CONSOLA] conectando con Kernel...");
    socket_kernel = crear_conexion_con_servidor(ConsolaConfig.IP_KERNEL, ConsolaConfig.PUERTO_KERNEL);
    log_info(logger, "socket_kernel:%d",socket_kernel);
    if (socket_kernel < 0)
    {
        log_info(logger, "[CONSOLA]: Error al conectar con Kernel. Finalizando Ejecucion");
        log_error(logger, "[CONSOLA]: Kernel no está disponible");

        return FAILURE;
    }
    log_info(logger, "[CONSOLA]: Conexion con Kernel: OK");
    enviar_mensaje_a_servidor("HOLA! SOY CONSOLA (☞ﾟヮﾟ)☞", socket_kernel);

    return SUCCESS;
}

int desconectar_con_kernel()
{
    log_info(logger, "[CONSOLA] Desconectando Kernel...");

    if (socket_kernel >= 0)
    {
        if (shutdown(socket_kernel, SHUT_RDWR) != 0)
        {
            log_error(logger, "[CONSOLA]: Error al desconectar Kernel");

            return FAILURE;
        }

        liberar_conexion_con_servidor(socket_kernel);
    }

    log_info(logger, "[CONSOLA] Desconectando Kernel: OK");

    return SUCCESS;
}

void terminar_consola()
{
    log_warning(logger, "[CONSOLA] Finalizando Ejecucion...");
    config_destroy(config);
    log_destroy(logger);
}