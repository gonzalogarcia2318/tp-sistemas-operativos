#include "kernel_utils.h"

Logger *logger;
Config *config;
Hilo hilo_consolas;
int socket_kernel;
int socket_cpu;
int socket_memoria;
int socket_file_system;

void iniciar_logger_kernel()
{
    logger = log_create(ARCHIVO_LOGGER, "Kernel", 1, LOG_LEVEL_INFO);
    log_info(logger, "[KERNEL]: Logger creado correctamente");
}

int iniciar_config_kernel(char* path)
{
    config = config_create(path);
    if(config == NULL)
    {
        log_error(logger,"[KERNEL]: ERROR AL INICIAR CONFIG INICIAL");
        return FAILURE;
    }
    rellenar_configuracion_kernel(config);
    log_info(logger, "[KERNEL]: Archivo Config creado y rellenado correctamente");
    return SUCCESS;
}

int iniciar_servidor_kernel()
{
    log_info(logger, "[KERNEL]: Iniciando Servidor ...");
    socket_kernel = iniciar_servidor(KernelConfig.IP, KernelConfig.PUERTO_ESCUCHA);

    if (socket_kernel < 0)
    {
        log_error(logger, "[KERNEL]: Error intentando iniciar Servidor.");
        return FAILURE;
    }

    log_info(logger, "[KERNEL]: Servidor iniciado correctamente.");
    return SUCCESS;
}

void conectar_con_consola()
{
    // Utiliza socket_kernel

    pthread_create(&hilo_consolas, NULL, (void *)esperar_consola, (void *)socket_kernel);
    pthread_detach(hilo_consolas);
}

int conectar_con_cpu()
{
    log_info(logger, "[KERNEL] conectando con CPU...");

    socket_cpu = crear_conexion_con_servidor(KernelConfig.IP_CPU, KernelConfig.PUERTO_CPU);

    if (socket_cpu < 0)
    {
        log_error(logger, "[KERNEL]: CPU NO ESTÁ DISPONIBLE, FINALIZANDO EJECUCIÓN");
        return FAILURE;
    }
    log_info(logger, "[KERNEL]: Conexion con CPU: OK");
    enviar_mensaje_a_servidor("HOLA! SOY KERNEL╰(*°▽°*)╯", socket_cpu);
    return SUCCESS;
}

int conectar_con_memoria()
{
    log_info(logger, "[KERNEL] conectando con Memoria...");

    socket_memoria = crear_conexion_con_servidor(KernelConfig.IP_MEMORIA, KernelConfig.PUERTO_MEMORIA);

    if (socket_memoria < 0)
    {
        log_error(logger, "[KERNEL]: MEMORIA NO ESTÁ DISPONIBLE, FINALIZANDO EJECUCIÓN");
        return FAILURE;
    }
    log_info(logger, "[KERNEL]: Conexion con Memoria: OK");
    enviar_mensaje_a_servidor("HOLA! SOY KERNEL ╰(*°▽°*)╯", socket_memoria);
    return SUCCESS;
}

int conectar_con_file_system()
{
    log_info(logger, "[KERNEL] conectando con FILE SYSTEM...");

    socket_file_system = crear_conexion_con_servidor(KernelConfig.IP_FILESYSTEM, KernelConfig.PUERTO_FILESYSTEM);

    if (socket_file_system < 0)
    {
        log_error(logger, "[KERNEL]: FILE SYSTEM NO ESTÁ DISPONIBLE, FINALIZANDO EJECUCIÓN");
        return FAILURE;
    }
    log_info(logger, "[KERNEL]: Conexion con FILE SYSTEM: OK");
    enviar_mensaje_a_servidor("HOLA! SOY KERNEL╰(*°▽°*)╯", socket_file_system);
    return SUCCESS;
}

t_list* crear_recursos(char** recursos, char** instancias_recursos){
    log_info(logger, "[KERNEL]: Crear recursos");

    t_list *lista_recursos = list_create();

    int i = 0;
    
    while(recursos[i] != NULL){

        Recurso *recurso = malloc(sizeof(Recurso));
        recurso->nombre = recursos[i];
        recurso->instancias = atoi(instancias_recursos[i]);
        recurso->cola_block = queue_create();

        list_add(lista_recursos, recurso);
        i++;
    }    

    return lista_recursos;
}

void liberar_memoria_recursos(t_list* recursos){
    for (int i = 0; i < list_size(recursos); i++){
        Recurso* recurso = list_get(recursos, i);
        queue_destroy(recurso->cola_block);
        free(recurso);
    }
}