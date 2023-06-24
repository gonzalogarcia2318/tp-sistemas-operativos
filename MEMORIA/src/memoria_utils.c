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
t_list* procesos_globales;

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

    crear_lista_procesos_globales();
}

void crear_segmento_compartido()
{
    segmento_compartido = malloc(sizeof(SEGMENTO));
    segmento_compartido->id = 0;
    segmento_compartido->base = 0;
    segmento_compartido->limite = MemoriaConfig.TAM_SEGMENTO_0;
    segmento_compartido->validez = 1;
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó el SEGMENTO COMPARTIDO con éxito");
}

void crear_espacio_usuario()
{
    espacio_usuario = malloc(sizeof(MemoriaConfig.TAM_MEMORIA));
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó el ESPACIO DE USUARIO con éxito");

}

void crear_lista_huecos_libres()
{
    huecos_libres = list_create();

    SEGMENTO* hueco_0 = malloc(sizeof(SEGMENTO));
    hueco_0->base = MemoriaConfig.TAM_SEGMENTO_0;
    hueco_0->limite = MemoriaConfig.TAM_MEMORIA - MemoriaConfig.TAM_SEGMENTO_0;

    list_add(huecos_libres,hueco_0);
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó la LISTA DE HUECOS LIBRES con éxito");
}

void crear_lista_procesos_globales()
{
    procesos_globales = list_create();
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó la lista de PROCESOS GLOBALES con éxito");
}
///////////////////////////////////////////PROCESOS KERNEL////////////////////////////////////////////////////////////////

t_list* manejar_crear_proceso(int *proceso_id)
{
    BUFFER* buffer = recibir_buffer(socket_kernel);
    PROCESO_MEMORIA* proceso = malloc(sizeof(PROCESO_MEMORIA));
    int32_t pid;

    memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);
    log_info(logger,"RECIBI CREAR_PROCESO: PID:<%d>", pid); //ELIMINAR LUEGO DE VERIFICAR

    proceso->pid = pid;
    proceso->tabla_de_segmentos = crear_tabla_de_segmentos();

    *proceso_id = pid;

    log_warning(logger,"Creación de Proceso PID: <%d>", pid);
    free(buffer);
    return proceso->tabla_de_segmentos;
}

t_list* crear_tabla_de_segmentos()
{
    t_list* tabla_segmentos = list_create();
    list_add(tabla_segmentos, segmento_compartido);
    //EL TAMAÑO SE VERIFICA A LA HORA DE CREAR SEMGNETOS => SI size = CANT_SEGMENTOS => NO CREA SEGMENTO
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó la TABLA DE SEGMENTOS con éxito");
    return tabla_segmentos;
}
void enviar_tabla_de_segmentos_a_kernel(t_list* tabla_de_segmentos, int pid)
{
  PAQUETE* paquete = crear_paquete(CREAR_PROCESO);
  paquete->buffer = serializar_segmentos(tabla_de_segmentos, pid);
  enviar_paquete_a_cliente(paquete, socket_kernel);
  eliminar_paquete(paquete);
}

void manejar_finalizar_proceso()
{
    BUFFER* buffer = recibir_buffer(socket_kernel);
    int32_t pid;
    memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);
    log_info(logger,"RECIBI ELIMINAR_PROCESO: PID:<%d>", pid);

    // SALE UN ERROR -> LLEGA MUCHAS VECES A KERNEL QUE FINALIZO EL PROCESO?
    // obtener_proceso_de_globales(pid); no devuelve nada
    /*PROCESO_MEMORIA* proceso = obtener_proceso_de_globales(pid);

    SEGMENTO* seg_aux;
    
    int size = list_size(proceso->tabla_de_segmentos);
    list_remove(proceso->tabla_de_segmentos,0); //NO ELIMINO EL SEGMENTO GLOBAL

    for (int i = 1; i < size; i++)
    {
        seg_aux = list_get(proceso->tabla_de_segmentos, i);
        //eliminar_segmento(seg_aux) ELIMINAR DE ESPACIO DE USUARIO Y CAMBIAR EN HUECOS LIBRES : TODO
        //free(seg_aux);
    }
    list_destroy(proceso->tabla_de_segmentos);
    eliminar_proceso_de_globales(pid);
    free(proceso);

    log_warning(logger,"Eliminación de Proceso PID: <%d>", pid);*/
}

PROCESO_MEMORIA* obtener_proceso_de_globales(int32_t pid)
{
    int size = list_size(procesos_globales);
    PROCESO_MEMORIA* proc_mem_aux;

    for(int i = 0; i < size; i++)
    {
        if(list_get(procesos_globales, i) != NULL)
        {
            proc_mem_aux = list_get(procesos_globales, i);
            if(proc_mem_aux->pid == pid)
            return proc_mem_aux;
        }
    }
}
void eliminar_proceso_de_globales(int32_t pid)
{
    int size = list_size(procesos_globales);
    PROCESO_MEMORIA* proc_mem_aux;

    for(int i = 0; i < size; i++)
    {
        if(list_get(procesos_globales, i) != NULL)
        {
            proc_mem_aux = list_get(procesos_globales, i);
            if(proc_mem_aux->pid == pid)
            {
                list_replace(procesos_globales,i,NULL);
                free(proc_mem_aux);
            }  
        }  
    }
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

///////////////////////////////////////////CREAR SEGMENTO//////////////////////////////////////////////

int manejar_crear_segmento(int32_t pid, int32_t id_segmento, int32_t tamanio_segmento)
{
    t_list* tabla_de_segmentos = obtener_tabla_de_segmentos(pid);

    if(!puedo_crear_nuevo_segmento_proceso(tabla_de_segmentos))
        return 3; //ERROR "Out of Memory" por tabla de segmentos llena.

    int caso = hay_espacio_memoria(tamanio_segmento);
    
    switch (caso)
    {
    case 1:
        //OK -> ASIGNAR SEGÚN ALGORITMO ASIGNACIÓN 
        int dire_base = crear_segmento(pid, id_segmento, tamanio_segmento);
        log_warning(logger,"PID: <%d> - Crear Segmento: ID SEGMENTO:<%d> - Base: <%d> - TAMAÑO: <%d>",
                    pid,
                    id_segmento,
                    dire_base,
                    tamanio_segmento
                    );
        return 1;
    case 2:
        //HAY QUE CONSOLIDAR -> AVISAR A KERNEL 
        return 2;
    case 3:
        //ERROR: OUT OF MEMORY -> AVISAR A KERNEL
        return 3;
    default:
        break;
    }
}

int crear_segmento(int32_t pid, int32_t id_segmento, int32_t tamanio_segmento)
{
    SEGMENTO* segmento_nuevo = malloc(sizeof(SEGMENTO));
    segmento_nuevo->id = id_segmento;
    segmento_nuevo->limite = tamanio_segmento;
    segmento_nuevo->validez = 1;

    int base = aplicar_algoritmo_asignacion(tamanio_segmento); // TODO

    segmento_nuevo->base = base;

    t_list* tabla_de_segmentos = obtener_tabla_de_segmentos(pid);
    list_add(tabla_de_segmentos,segmento_nuevo);

    return base;
}

int aplicar_algoritmo_asignacion(int32_t tamanio_segmento)
{
    return 0; //TODO
}

bool puedo_crear_nuevo_segmento_proceso(t_list* tabla_de_segmentos)
{
    int max_segmentos = MemoriaConfig.CANT_SEGMENTOS;
    int size = list_size(tabla_de_segmentos);

    if(size < max_segmentos)
        return true;
    
    SEGMENTO* seg_aux;

    for(int i = 0; i < size; i++) //PUEDE HABER NO VALIDOS 
    {
        seg_aux = list_get(tabla_de_segmentos,i);
        if(seg_aux->validez != 1)
            size--;  
    }

    if(size < max_segmentos)
        return true;

    log_error(logger,"El proceso no puede crear nuevos segmentos, alcanzo el máximo");
    return false;
}

int hay_espacio_memoria(int32_t tamanio_segmento)
{
    int size = list_size(huecos_libres);
    SEGMENTO* seg_aux;

    for (int i = 0; i < size; i++)
    {
        seg_aux = list_get(huecos_libres,i);
        if(seg_aux->limite >= tamanio_segmento)
            return 1; //HAY ESPACIO CONTIGUO: OK
    }

    int espacio_acumulado = 0;

    for (int i = 0; i < size; i++)
    {
        seg_aux = list_get(huecos_libres,i);
        espacio_acumulado += seg_aux->limite;
    }

    if(espacio_acumulado >= tamanio_segmento)
        return 2; //HAY QUE CONSOLIDAR
    
    else
        return 3; //FALTA ESPACIO
    
}

t_list* obtener_tabla_de_segmentos(int32_t pid)
{
    PROCESO_MEMORIA* proceso = obtener_proceso_de_globales(pid);
    t_list* tabla_de_segmentos = proceso->tabla_de_segmentos;
    return tabla_de_segmentos;
}

void eliminar_segmento()
{
    //TODO
}