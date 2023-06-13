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

SEGMENTO* segmento_compartido;
t_list* huecos_libres;
void* espacio_usuario;

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
///////////////////////////////////////////ESTRUCTURAS ADMINISTRATIVAS/////////////////////////////////////////

void crear_estructuras_administrativas()
{
    crear_segmento_compartido();
    
    crear_espacio_usuario();

    crear_lista_huecos_libres();
}

void crear_segmento_compartido()
{
    //CREAR STRUCT SEGMENTO PÚBLICO CON LOS SIGUIENTES DATOS:
        //ID = 0
        //BASE = 0
        //LÍMITE = TAM_MAXIMO
    segmento_compartido = malloc(sizeof(SEGMENTO));
    segmento_compartido->id = 0;
    segmento_compartido->base = 0;
    segmento_compartido->limite = MemoriaConfig.TAM_SEGMENTO_0;
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó el SEGMENTO COMPARTIDO con éxito");
}

void crear_espacio_usuario()
{
    void* espacio_usuario = malloc(sizeof(MemoriaConfig.TAM_MEMORIA));
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó el ESPACIO DE USUARIO con éxito");

}

void crear_lista_huecos_libres()
{
    huecos_libres = list_create();

    SEGMENTO* hueco_0 = malloc(sizeof(SEGMENTO));
    hueco_0->base = MemoriaConfig.TAM_SEGMENTO_0;
    hueco_0->limite = MemoriaConfig.TAM_MEMORIA - 1;

    list_add(huecos_libres,hueco_0);
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó la LISTA DE HUECOS LIBRES con éxito");
}

t_list* crear_tabla_de_segmentos()
{
    t_list* tabla_segmentos = list_create();
    list_add(tabla_segmentos, segmento_compartido);
    //EL TAMAÑO SE VERIFICA A LA HORA DE CREAR SEMGNETOS => SI size = CANT_SEGMENTOS => NO CREA SEGMENTO
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó la TABLA DE SEGMENTOS con éxito");
    return tabla_segmentos;
}

//////////////////////////////////////////////ACCESO A ESPACIO DE USUARIO////////////////////////////////////

char* leer_de_memoria(int32_t direccion_fisica, int32_t bytes_registro)
{
    void* posicion = espacio_usuario + direccion_fisica;
    int tamanio = (bytes_registro + 1) * sizeof(char);

    char* contenido = (char*)malloc(tamanio);
    
    if(contenido == NULL)
        log_error(logger,"ERROR AL RESERVAR MEMORIA PARA EL CONTENIDO DENTRO DE leer_de_memoria()");
    
    memcpy(contenido,posicion,tamanio);

    log_info(logger,"LEÍ DE MEMORIA EL CONTENIDO:<%s>, DE LA DIRECCIÓN FÍSICA:<%d> y DE TAMANIO:<%d>",
                     contenido,
                     direccion_fisica,
                     tamanio
            );

    aplicar_retardo_espacio_usuario();

    return contenido;
}
void escribir_en_memoria(char* contenido, int32_t direccion_fisica, int32_t bytes_registro)
{
    void* posicion = espacio_usuario + direccion_fisica;

    memcpy(posicion,contenido,(bytes_registro+1)*sizeof(char));

    log_info(logger,"ESCRIBÍ EN MEMORIA EL CONTENIDO:<%s> EN LA DIRECCIÓN FÍSICA:<%d>",
                     contenido,
                     direccion_fisica
            );
    aplicar_retardo_espacio_usuario();
}

void aplicar_retardo_espacio_usuario()
{
    int segundos = MemoriaConfig.RETARDO_MEMORIA/1000;
    log_info(logger,"Retraso de <%d> segundos por acceso a espacio de usuario",segundos);
    sleep(segundos);
}