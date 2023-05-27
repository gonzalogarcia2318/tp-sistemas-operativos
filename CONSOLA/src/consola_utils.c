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

void inicializar_config_consola(char *path_config)
{
    config = config_create(path_config);
    rellenar_configuracion_consola(config);
    log_info(logger, "[CONSOLA]: Archivo Config creado y rellenado correctamente");
}

int conectar_con_kernel()
{
    log_info(logger, "[CONSOLA] conectando con Kernel...");
    socket_kernel = crear_conexion_con_servidor(ConsolaConfig.IP_KERNEL, ConsolaConfig.PUERTO_KERNEL);
    log_info(logger, "socket_kernel:%d", socket_kernel);
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

t_list *leer_instrucciones(char *path_instrucciones)
{
    t_list *instrucciones = list_create();
    char linea[100];

    FILE *archivo_instrucciones = fopen(path_instrucciones, "r");
    if (archivo_instrucciones == NULL)
    {
        log_error(logger, "[CONSOLA]: No se pudo leer el archivo de pseudocodigo.");
        exit(EXIT_FAILURE);
    }

    while (fgets(linea, sizeof(linea), archivo_instrucciones))
    {
        log_info(logger, "Linea leida22: %s", linea);

        Instruccion *instruccion = parsear_instruccion_por_linea(linea);

        list_add(instrucciones, instruccion);
    }

    fclose(archivo_instrucciones);

    log_info(logger, "1SIZE %d", list_size(instrucciones));

    return instrucciones;
}

Instruccion *parsear_instruccion_por_linea(char *linea)
{
    char **linea_splitted = string_split(linea, " ");

    // TODO: free(instruccion) cuando se termine de usar
    Instruccion *instruccion = malloc(sizeof(Instruccion));
    instruccion->nombreArchivo = "";
    instruccion->nombreInstruccion = "";
    instruccion->valor = "";
    instruccion->registro = "";
    instruccion->recurso = "";

    instruccion->nombreInstruccion = linea_splitted[0];

    if (strcmp(instruccion->nombreInstruccion, "SET") == 0)
    {
        instruccion->registro = linea_splitted[1];
        instruccion->valor = linea_splitted[2];
    }
    else if (strcmp(instruccion->nombreInstruccion, "MOV_IN") == 0)
    {
        instruccion->registro = linea_splitted[1];
        instruccion->direccionLogica = atoi(linea_splitted[2]);
    }
    else if (strcmp(instruccion->nombreInstruccion, "MOV_OUT") == 0)
    {
        instruccion->direccionLogica = atoi(linea_splitted[1]);
        instruccion->registro = linea_splitted[2];
    }
    else if (strcmp(instruccion->nombreInstruccion, "I/O") == 0)
    {
        instruccion->tiempo = atoi(linea_splitted[1]);
    }
    else if (strcmp(instruccion->nombreInstruccion, "F_OPEN") == 0 || strcmp(instruccion->nombreInstruccion, "F_CLOSE") == 0)
    {
        instruccion->nombreArchivo = linea_splitted[1];
    }
    else if (strcmp(instruccion->nombreInstruccion, "F_SEEK") == 0)
    {
        instruccion->nombreArchivo = linea_splitted[1];
        instruccion->posicion = atoi(linea_splitted[2]);
    }
    else if (strcmp(instruccion->nombreInstruccion, "F_READ") == 0 || strcmp(instruccion->nombreInstruccion, "F_WRITE") == 0)
    {
        instruccion->nombreArchivo = linea_splitted[1];
        instruccion->direccionLogica = atoi(linea_splitted[2]);
        instruccion->cantBytes = atoi(linea_splitted[3]);
    }
    else if (strcmp(instruccion->nombreInstruccion, "F_TRUNCATE") == 0)
    {
        instruccion->nombreArchivo = linea_splitted[1];
        instruccion->valor = atoi(linea_splitted[2]);
    }
    else if (strcmp(instruccion->nombreInstruccion, "WAIT") == 0 || strcmp(instruccion->nombreInstruccion, "SIGNAL") == 0)
    {
        instruccion->recurso = linea_splitted[1];
    }
    else if (strcmp(instruccion->nombreInstruccion, "CREATE_SEGMENT") == 0)
    {
        instruccion->idSegmento = atoi(linea_splitted[1]);
        instruccion->valor = atoi(linea_splitted[2]);
    }
    else if (strcmp(instruccion->nombreInstruccion, "DELETE_SEGMENT") == 0)
    {
        instruccion->idSegmento = atoi(linea_splitted[1]);
    }

    free(linea_splitted);

    return instruccion;
}