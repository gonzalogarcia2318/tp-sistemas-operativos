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
t_list* tabla_de_segmentos_globales;

void iniciar_logger_memoria()
{
    logger = log_create(ARCHIVO_LOGGER, "Memoria", 1, LOG_LEVEL_INFO);
    log_info(logger, "[MEMORIA]: Logger creado correctamente");
}

int iniciar_config_memoria(char* path)
{
    config = config_create(path);
    if(config == NULL)
    {
        log_error(logger,"[MEMORIA]: ERROR AL INICIAR CONFIG INICIAL");
        return FAILURE;
    }
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

///////////////////////////////////////////ESTRUCTURAS ADMINISTRATIVAS/////////////////////////////////////////

void crear_estructuras_administrativas()
{
    crear_segmento_compartido();
    
    crear_espacio_usuario();

    crear_lista_huecos_libres();

    crear_lista_procesos_globales();

    crear_tabla_segmentos_globales();
}

void crear_segmento_compartido()
{
    segmento_compartido = malloc(sizeof(SEGMENTO));
    segmento_compartido->pid = -1;
    segmento_compartido->id = 0;
    segmento_compartido->base = 0;
    segmento_compartido->limite = MemoriaConfig.TAM_SEGMENTO_0;
    //segmento_compartido->validez = 1;
    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó el SEGMENTO COMPARTIDO con éxito");
}

void crear_espacio_usuario()
{
    espacio_usuario = malloc(MemoriaConfig.TAM_MEMORIA);
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

void crear_tabla_segmentos_globales()
{
    tabla_de_segmentos_globales = list_create();

    list_add(tabla_de_segmentos_globales,segmento_compartido);

    log_info(logger,"ESTRUCTURAS ADMINISTRATIVAS: Se creó la TABLA DE SEGMENTOS GLOBALES con éxito");
}
///////////////////////////////////////////PROCESOS KERNEL////////////////////////////////////////////////////////////////

t_list* manejar_crear_proceso()
{
    BUFFER* buffer = recibir_buffer(socket_kernel);
    void * buffer_stream_inicial = buffer->stream;


    PROCESO_MEMORIA* proceso = malloc(sizeof(PROCESO_MEMORIA));
    int32_t pid;

    memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

    proceso->pid = pid;
    proceso->tabla_de_segmentos = crear_tabla_de_segmentos();

    list_add(procesos_globales, proceso);

    log_warning(logger,"Creación de Proceso PID: <%d>", pid);

    free(buffer_stream_inicial);
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

void enviar_tabla_de_segmentos_a_kernel(t_list* tabla_de_segmentos)
{
    PAQUETE* paquete = crear_paquete(CREAR_PROCESO);
    paquete->buffer = serializar_segmentos(tabla_de_segmentos);
    enviar_paquete_a_cliente(paquete, socket_kernel);
    log_info(logger, "Enviar tabla de segmentos a KERNEL por CREAR_PROCESO");
    eliminar_paquete(paquete);
}

void enviar_tabla_de_segmentos_a_kernel_por_delete_segment(t_list* tabla_de_segmentos)
{
    PAQUETE* paquete = crear_paquete(BORRAR_SEGMENTO);
    paquete->buffer = serializar_segmentos(tabla_de_segmentos);
    log_info(logger, "Enviar tabla de segmentos a KERNEL por DELETE_SEGMENT");
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void enviar_tabla_de_segmentos_a_kernel_despues_de_consolidar(t_list* tabla_de_segmentos)
{
    PAQUETE* paquete = crear_paquete(CONSOLIDAR);
    paquete->buffer = serializar_segmentos(tabla_de_segmentos);
    log_info(logger, "Envié tabla de segmentos a KERNEL por FIN DE CONSOLIDACION");
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void manejar_finalizar_proceso()
{
    BUFFER* buffer = recibir_buffer(socket_kernel);
    void* buffer_stream_inicial = buffer->stream;

    int32_t pid;
    memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

    t_list* tabla_segmentos_proceso = obtener_tabla_de_segmentos(pid);

    int size = list_size(tabla_segmentos_proceso);
    SEGMENTO* seg_aux;
    
    for (int i = size; i > 1; i--) //NO ELIMINO EL SEGMENTO COMPARTIDO
    {
        seg_aux = list_get(tabla_segmentos_proceso, i-1);
        manejar_eliminar_segmento(seg_aux); // REDIMENSIONA HUECOS. ELIMsiNA EL SEGMENTO DE TABLA LOCAL Y GLOBAL. SIN FREE
        free(seg_aux);
    }
    list_destroy(tabla_segmentos_proceso);
    eliminar_proceso_de_globales(pid); //FREE INCLUIDO
    log_warning(logger,"Eliminación de Proceso PID: <%d>", pid);

    free(buffer_stream_inicial);
    free(buffer);
}

PROCESO_MEMORIA* obtener_proceso_de_globales(int32_t pid)
{
    int size = list_size(procesos_globales);
    PROCESO_MEMORIA* proc_mem_aux;

    for(int i = 0; i < size; i++)
    {
        proc_mem_aux = list_get(procesos_globales, i);

        if(proc_mem_aux->pid == pid)
        {
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
    
        proc_mem_aux = list_get(procesos_globales, i);
  
        if(proc_mem_aux->pid == pid)
        {
            list_remove(procesos_globales,i);
            free(proc_mem_aux);
            return;
        }  
    }
}

//////////////////////////////////////////////ACCESO A ESPACIO DE USUARIO////////////////////////////////////

char* leer_de_memoria(int32_t direccion_fisica, int32_t bytes_registro)
{
    void* posicion = espacio_usuario + direccion_fisica;
    int tamanio = (bytes_registro) * sizeof(char);

    char* contenido = malloc(tamanio);
    
    if(contenido == NULL)
        log_error(logger,"ERROR AL RESERVAR MEMORIA PARA EL CONTENIDO DENTRO DE leer_de_memoria()");
    
    memcpy(contenido,posicion,tamanio);

    contenido[tamanio] = '\0';

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

    memcpy(posicion,contenido,bytes_registro);

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

int32_t manejar_crear_segmento(int32_t pid, int32_t id_segmento, int32_t tamanio_segmento)
{
    t_list* tabla_de_segmentos = obtener_tabla_de_segmentos(pid);

    if(!puedo_crear_nuevo_segmento_proceso(tabla_de_segmentos))
    {
        log_error(logger,"NO PUEDO CREAR NUEVO SEGMNETO, -3");
        return -3; //ERROR "Out of Memory" por tabla de segmentos llena.
    }
        

    int caso = hay_espacio_memoria(tamanio_segmento);
    
    switch (caso)
    {
    case 1:
        //OK -> ASIGNAR SEGÚN ALGORITMO ASIGNACIÓN 
        int32_t dire_base = crear_segmento(pid, id_segmento, tamanio_segmento);
        log_warning(logger,"PID: <%d> - Crear Segmento: ID SEGMENTO: <%d> - BASE: <%d> - TAMAÑO: <%d>",
                    pid,
                    id_segmento,
                    dire_base,
                    tamanio_segmento
                    );
        return dire_base;
    case 2:
        //HAY QUE CONSOLIDAR -> AVISAR A KERNEL 
        return -2;
    case 3:
        //ERROR: OUT OF MEMORY -> AVISAR A KERNEL
        log_error(logger,"FALTA MEMORIA, SALGO POR -3 DE CREAR SEGMENTO");
        return -3;
    default:
        break;
    }
}


bool puedo_crear_nuevo_segmento_proceso(t_list* tabla_de_segmentos)
{
    int max_segmentos = MemoriaConfig.CANT_SEGMENTOS;
    int size = list_size(tabla_de_segmentos);

    if(size < max_segmentos)
        return true;
    
    /*
    SEGMENTO* seg_aux;

    for(int i = 0; i < size; i++) //PUEDE HABER NO VALIDOS 
    {
        seg_aux = list_get(tabla_de_segmentos,i);
        if(seg_aux->validez != 1)
            size--;  
    }

    if(size < max_segmentos)
        return true;
    */

    log_error(logger,"El proceso no puede crear nuevos segmentos, alcanzo el máximo");
    return false;
}

int hay_espacio_memoria(int32_t tamanio_segmento)
{
    int size = list_size(huecos_libres);
    SEGMENTO* hueco_aux;
    
    for (int i = 0; i < size; i++)
    {
        hueco_aux = list_get(huecos_libres,i);
        if(hueco_aux->limite >= tamanio_segmento)
            return 1; //HAY ESPACIO CONTIGUO: OK
    }

    int espacio_acumulado = 0;

    for (int i = 0; i < size; i++)
    {
        hueco_aux = list_get(huecos_libres,i);
        espacio_acumulado += hueco_aux->limite;
    }

    if(espacio_acumulado >= tamanio_segmento)
        return 2; //HAY QUE CONSOLIDAR
    
    else
        return 3; //FALTA ESPACIO
    
}

int32_t crear_segmento(int32_t pid, int32_t id_segmento, int32_t tamanio_segmento)
{
    SEGMENTO* segmento_nuevo = malloc(sizeof(SEGMENTO));
    segmento_nuevo->pid = pid;
    segmento_nuevo->id = id_segmento;
    segmento_nuevo->limite = tamanio_segmento;
    //segmento_nuevo->validez = 1;

    int32_t base = aplicar_algoritmo_asignacion(tamanio_segmento);

    segmento_nuevo->base = base;

    t_list* tabla_de_segmentos = obtener_tabla_de_segmentos(pid);
    
    agregar_segmento_a_tabla_global(segmento_nuevo);

    list_add(tabla_de_segmentos,segmento_nuevo);

    return base;
}

int32_t aplicar_algoritmo_asignacion(int32_t tamanio_segmento)
{
    int32_t base = 0;

    if(!strcmp(MemoriaConfig.ALGORITMO_ASIGNACION,"FIRST"))
    {
        base = aplicar_algoritmo_asignacion_FIRST(tamanio_segmento);
        return base;
    }
    else if(!strcmp(MemoriaConfig.ALGORITMO_ASIGNACION,"BEST"))
    {
        base = aplicar_algoritmo_asignacion_BEST(tamanio_segmento);
        return base;
    }
    else if(!strcmp(MemoriaConfig.ALGORITMO_ASIGNACION,"WORST"))
    {
        base = aplicar_algoritmo_asignacion_WORST(tamanio_segmento);
        return base;
    }
    else
    {
        log_error(logger,"ALGORITMO DE ASIGNACIÓN DESCONOCIDO");
        return base;
    }
}

int32_t aplicar_algoritmo_asignacion_FIRST(int32_t tamanio_segmento)
{
    int size = list_size(huecos_libres);
    int base = 0;

    for (int i = 0; i < size; i++)
    {
        SEGMENTO* hueco = list_get(huecos_libres,i);
        if(hueco->limite == tamanio_segmento) //SEGMENTO CALZA JUSTO EN EL HUECO
        {
            base = hueco->base;
            eliminar_hueco(hueco->base);
            return base;
        }
        else if(hueco->limite > tamanio_segmento)
        {
            base = hueco->base;
            hueco->base +=tamanio_segmento;
            hueco->limite-=tamanio_segmento;
            return base;
        }
    }

    log_error(logger,"NO ENCONTRE UN HUECO DISPONIBLE AL aplicar_algoritmo_asignación_FIRST(1)");//NO DEBERÍA PASAR PORQUE LO VERIFICA ANTES
    return 0;
}

int32_t aplicar_algoritmo_asignacion_BEST(int32_t tamanio_segmento)
{
    int size = list_size(huecos_libres);
    int32_t base = 0;
    SEGMENTO* hueco_menor = NULL;

    for (int i = 0; i < size; i++)
    {
        SEGMENTO* hueco_aux = list_get(huecos_libres,i);
        if(hueco_aux->limite >= tamanio_segmento)
        {
            if(hueco_menor == NULL)
            {
                hueco_menor = hueco_aux;
            }
            else if(hueco_aux->limite < hueco_menor->limite)
            {
                hueco_menor = hueco_aux;
            }
        }
    }

    if(hueco_menor == NULL)
    {
        log_error(logger,"NO ENCONTRE UN HUECO DISPONIBLE AL aplicar_algoritmo_asignación_BEST(1)"); //NO DEBERÍA PASAR PORQUE LO VERIFICA ANTES
        return 0;
    }

    if(hueco_menor->limite == tamanio_segmento) //SEGMENTO CALZA JUSTO EN EL HUECO
    {
        base = hueco_menor->base;
        eliminar_hueco(hueco_menor->base);
        return base;
    }
    else
    {
        base = hueco_menor->base;
        hueco_menor->base +=tamanio_segmento;
        hueco_menor->limite-=tamanio_segmento;
        return base;
    }
}
    

int32_t aplicar_algoritmo_asignacion_WORST(int32_t tamanio_segmento)
{
    int size = list_size(huecos_libres);
    int32_t base = 0;
    SEGMENTO* hueco_mayor = NULL;

    for (int i = 0; i < size; i++)
    {
        SEGMENTO* hueco_aux = list_get(huecos_libres,i);
        if(hueco_aux->limite >= tamanio_segmento)
        {
            if(hueco_mayor == NULL)
            {
                hueco_mayor = hueco_aux;
            }
            else if(hueco_aux->limite > hueco_mayor->limite)
            {
                hueco_mayor = hueco_aux;
            }
        }
    }

    if(hueco_mayor == NULL)
    {
        log_error(logger,"NO ENCONTRE UN HUECO DISPONIBLE AL aplicar_algoritmo_asignación_WORST(1)"); //NO DEBERÍA PASAR PORQUE LO VERIFICA ANTES
        return 0;
    }

    if(hueco_mayor->limite == tamanio_segmento) //SEGMENTO CALZA JUSTO EN EL HUECO
    {
        base = hueco_mayor->base;
        eliminar_hueco(hueco_mayor->base);
        return base;
    }
    else
    {
        base = hueco_mayor->base;
        hueco_mayor->base +=tamanio_segmento;
        hueco_mayor->limite-=tamanio_segmento;
        return base;
    }
}

void agregar_segmento_a_tabla_global(SEGMENTO* segmento) //AÑADE EN ORDEN (MENOR A MAYOR BASE)
{
    bool esMayor(SEGMENTO* seg_men, SEGMENTO* seg_may)
    {
        return seg_men->base < seg_may->base;
    }

    list_add_sorted(tabla_de_segmentos_globales,segmento,(void*)esMayor);
}


t_list* obtener_tabla_de_segmentos(int32_t pid)
{
    PROCESO_MEMORIA* proceso = obtener_proceso_de_globales(pid);
    t_list* tabla_de_segmentos = proceso->tabla_de_segmentos;
    return tabla_de_segmentos;
}

SEGMENTO* obtener_segmento_de_tabla_de_segmentos(t_list* tabla_de_segmentos,int32_t id_segmento)
{
    int size = list_size(tabla_de_segmentos);
    SEGMENTO* seg_buscado;

    for (int i = 0; i < size; i++)
    {
        seg_buscado = list_get(tabla_de_segmentos,i);
        if(seg_buscado->id == id_segmento)
        {
            return seg_buscado;
        }
    }
    return NULL;
}

void manejar_eliminar_segmento(SEGMENTO* segmento)
{
    int32_t base = segmento->base;
    int32_t limite = segmento->limite;

    redimensionar_huecos_eliminar_segmento(base, limite);

    eliminar_segmento_de_globales(segmento->id);

    eliminar_segmento_de_tabla_segmentos_proceso(segmento->pid, segmento->id);
}

void redimensionar_huecos_eliminar_segmento(int32_t base_segmento, int32_t limite_segmento)
{
    int size = list_size(huecos_libres);
   
    int tiene_hueco_detras = 0;
    int tiene_hueco_delante = 0;

    SEGMENTO* hueco_detras = NULL;
    SEGMENTO* hueco_delante = NULL;
    SEGMENTO* hueco_aux;

    for (int i = 0; i < size; i++)
    {
        hueco_aux = list_get(huecos_libres,i);

        if((hueco_aux->base + hueco_aux->limite) == base_segmento) //TIENE HUECO DETRAS
        {
            tiene_hueco_detras = 1;
            hueco_detras = hueco_aux;
        }

        if(hueco_aux->base == (base_segmento + limite_segmento)) //TIENE HUECO DELANTE
        {
            tiene_hueco_delante = 1;
            hueco_delante = hueco_aux;
        }

        if(tiene_hueco_delante && tiene_hueco_detras) break;
    }
    
    if(tiene_hueco_delante && tiene_hueco_detras) //CONSOLIDAR HUECOS
    {
        hueco_detras->limite += limite_segmento;
        hueco_detras->limite += hueco_delante->limite;

        eliminar_hueco(hueco_delante->base); //FREE INCLUIDO
        log_info(logger,"ELIMINÉ UN HUECO POR MOTIVO DE ELIMINAR SEGMENTO, DADO QUE CONSOLIDÉ DOS HUECOS");
        return;
    }
    else if(tiene_hueco_delante)//TOMA EL LUGAR DE LA BASE DEL SEGMENTO Y OCUPA SU ESPACIO
    {
        hueco_delante->base = base_segmento;
        hueco_delante->limite += limite_segmento;
        return;
    }
    else if(tiene_hueco_detras)//OCUPA SU ESPACIO
    {
        hueco_detras->limite += limite_segmento;
        return;
    }
    else //NO TIENE HUECOS ALEDAÑIOS => CREO UNO.
    {
        SEGMENTO* hueco = malloc(sizeof(SEGMENTO));
        hueco->base = base_segmento;
        hueco->limite = limite_segmento;

        list_add(huecos_libres,hueco);
        log_info(logger,"CREÉ UN HUECO POR MOTIVO DE ELIMINAR SEGMENTO, DADO QUE NO POSEE HUECOS ALEDAÑOS");
        return;
    }
}

void eliminar_hueco(int32_t base_hueco)
{
    int size = list_size(huecos_libres);
    SEGMENTO* hueco_aux;

    for (int i = 0; i < size; i++)
    {
       hueco_aux = list_get(huecos_libres,i);

       if(hueco_aux->base == base_hueco)
       {
        list_remove(huecos_libres,i);
        free(hueco_aux);
        return;
       }
    }
}

void eliminar_segmento_de_globales(int32_t id_segmento)
{
    int size = list_size(tabla_de_segmentos_globales);
    SEGMENTO* seg_aux;

    for (int i = 0; i < size; i++)
    {
        seg_aux = list_get(tabla_de_segmentos_globales,i);

        if(seg_aux->id == id_segmento)
        {
            list_remove(tabla_de_segmentos_globales,i);
            log_info(logger,"Se ha eliminado el segmento: <%d> de globales con motivo de eliminar segmento",id_segmento);
            return;
        }
    }
}
void eliminar_segmento_de_tabla_segmentos_proceso(int32_t pid, int32_t id_segmento)
{
    t_list* tabla_segmentos = obtener_tabla_de_segmentos(pid);
    int size = list_size(tabla_segmentos);
    SEGMENTO* seg_aux;

    for (int i = 0; i < size; i++)
    {
        seg_aux = list_get(tabla_segmentos,i);

        if(seg_aux->id == id_segmento)
        {
            list_remove(tabla_segmentos,i);
            log_info(logger,"Se ha eliminado el segmento: <%d> del proceso: <%d> de locales con motivo de eliminar segmento",id_segmento,pid);
            return;
        }
    }
}

///////////////////////////////////////////////////////////COMPACTAR//////////////////////////////////////////////////////////////////////////////////

void compactar()
{
    int size = list_size(tabla_de_segmentos_globales);
    SEGMENTO* seg_aux_anterior;
    SEGMENTO* seg_aux_actual;
    SEGMENTO* segmento_test;
    int32_t nueva_base;

    imprimir_tabla_segmentos_globales();

    for (int i = 1; i < size; i++)
    {
        seg_aux_anterior = (SEGMENTO*)list_get(tabla_de_segmentos_globales,i-1);
        seg_aux_actual = (SEGMENTO*)list_get(tabla_de_segmentos_globales,i);

        if(seg_aux_anterior->base + seg_aux_anterior->limite != seg_aux_actual->base) //SI HAY HUECO ENTRE ELLOS
        {
            nueva_base = seg_aux_anterior->base + seg_aux_anterior->limite;

            leer_y_escribir_memoria(nueva_base, seg_aux_actual);

            redimensionar_huecos_compactar(seg_aux_actual->base, seg_aux_actual->limite);

            seg_aux_actual->base = nueva_base;
        }
    }
    
    aplicar_retardo_compactacion();

    imprimir_tabla_segmentos_globales();
}

void leer_y_escribir_memoria(int32_t base_nueva, SEGMENTO* segmento)
{   
    char* leido = malloc(segmento->limite);

    strcpy(leido, leer_de_memoria(segmento->base,segmento->limite));
    
    escribir_en_memoria(leido, base_nueva, segmento->limite);

    free(leido);

}

void redimensionar_huecos_compactar(int32_t base_segmento, int32_t limite_segmento)
{
    int size = list_size(huecos_libres);
   
    int tiene_hueco_detras = 0;
    int tiene_hueco_delante = 0;

    SEGMENTO* hueco_detras = NULL;
    SEGMENTO* hueco_delante = NULL;
    SEGMENTO* hueco_aux;

    for (int i = 0; i < size; i++)
    {
        hueco_aux = (SEGMENTO*) list_get(huecos_libres,i);

        if((hueco_aux->base + hueco_aux->limite) == base_segmento) //TIENE HUECO DETRAS
        {
            tiene_hueco_detras = 1;
            hueco_detras = hueco_aux;
        }

        if(hueco_aux->base == (base_segmento + limite_segmento)) //TIENE HUECO DELANTE
        {
            tiene_hueco_delante = 1;
            hueco_delante = hueco_aux;
        }

        if(tiene_hueco_delante && tiene_hueco_detras) break;
    }
    if(tiene_hueco_delante && tiene_hueco_detras) //COMPACTAR HUECOS
    {
        hueco_detras->limite += hueco_delante->limite;
        hueco_detras->base += limite_segmento;

        eliminar_hueco(hueco_delante->base); //FREE INCLUIDO
        log_info(logger,"ELIMINÉ UN HUECO POR MOTIVO DE COMPACTAR SEGMENTO, DADO QUE CONSOLIDÉ DOS HUECOS");
        return;
    }
    else if(tiene_hueco_delante)//SE ENCARGA EL SEGMENTO SIGUIENTE
    {
        return;
    }
    else if(tiene_hueco_detras)//LE DEJA EL LUGAR
    {
        if((hueco_detras->limite == limite_segmento) && ((base_segmento + limite_segmento) == MemoriaConfig.TAM_MEMORIA)) //ÚLTIMO SEGMENTO ES IGUAL AL HUECO
        {
            hueco_detras->base = base_segmento;

        }
        else if(hueco_detras->limite == limite_segmento) //OCUPO TOTALMENTE EL HUECO => LO ELIMINO
        {
            eliminar_hueco(hueco_detras->base);
            log_info(logger,"ELIMINÉ UN HUECO POR MOTIVO DE CONSOLIDAR SEGMENTO, DADO QUE OCUPA TOTALMENTE EL MISMO");
        }
        else
        {
            hueco_detras->base += limite_segmento;
        }

        return;
    }
    else
    {
        log_error(logger,"NO ENCONTRÉ HUECOS AL redimensionar_huecos_compactar(2)");
        return;
    } 
}

void imprimir_tabla_segmentos_globales()
{
    int size = list_size(tabla_de_segmentos_globales);
    SEGMENTO* seg_aux;

    for (int i = 0; i < size; i++)
    {
        seg_aux = list_get(tabla_de_segmentos_globales,i);

        log_warning(logger, "PID:<%d>, SEGMENTO:<%d>, BASE:<%d>, TAMAÑO:<%d>",
                        seg_aux->pid,
                        seg_aux->id,
                        seg_aux->base,
                        seg_aux->limite
                );
        //leer_de_memoria(seg_aux->base,seg_aux->limite);
    }
}

void imprimir_tabla_segmentos_proceso(t_list* tabla_segmentos)
{
    int size = list_size(tabla_segmentos);
    SEGMENTO* seg_aux;

    for (int i = 0; i < size; i++)
    {
        seg_aux = list_get(tabla_segmentos,i);

        log_info(logger, "PID:<%d>, SEGMENTO:<%d>, BASE:<%d>, TAMAÑO:<%d>",
                        seg_aux->pid,
                        seg_aux->id,
                        seg_aux->base,
                        seg_aux->limite
                );
    }
}

void aplicar_retardo_compactacion()
{
    int segundos = MemoriaConfig.RETARDO_COMPACTACION/1000;
    log_info(logger,"Retraso de <%d> segundos por compactacion",segundos);
    sleep(segundos);
}
//////////////////////////////COMUNICACIÓN CON FILE SYSTEM///////////////////////////////

char* manejar_read_file_system()
{
    BUFFER* buffer = recibir_buffer(socket_file_system);
    void * buffer_stream_inicial = buffer->stream;

    int32_t direccion_fisica;
    int32_t tamanio;
    int32_t pid;
    char* leido;
    
    memcpy(&direccion_fisica, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
    memcpy(&tamanio, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);
    memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
     
    
    leido = malloc(tamanio);

    strcpy(leido,leer_de_memoria(direccion_fisica, tamanio));

    log_warning(logger,"ACCESO A ESPACIO DE USUARIO: PID:<%d> - ACCIÓN: <LEER> - DIRECCIÓN FÍSICA:<%d> - TAMAÑO:<%d> - ORIGEN: <FS>", 
                    pid,
                    direccion_fisica,
                    tamanio
                ); 

    free(buffer_stream_inicial);
    free(buffer);

    return leido;
}

void manejar_write_file_system()
{
    BUFFER* buffer = recibir_buffer(socket_file_system);
    void * buffer_stream_inicial = buffer->stream;


    int32_t direccion_fisica;
    int32_t tamanio;

    int32_t pid;
    
    memcpy(&direccion_fisica, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
    memcpy(&tamanio, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 

    char* valor_a_escribir = malloc(sizeof(char) * (tamanio));

    memcpy(valor_a_escribir, buffer->stream + sizeof(int32_t), sizeof(char) * (tamanio));
            buffer->stream += (tamanio); 
  
    memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); 
    
    escribir_en_memoria(valor_a_escribir, direccion_fisica, tamanio);

    log_warning(logger,"ACCESO A ESPACIO DE USUARIO: PID:<%d> - ACCIÓN: <ESCRIBIR> - DIRECCIÓN FÍSICA:<%d> - TAMAÑO:<%d> - ORIGEN: <FS>", 
                    pid,
                    direccion_fisica,
                    tamanio
                ); 
    free(buffer_stream_inicial);
    free(buffer);
    
    free(valor_a_escribir);
}

//////////////////////////////////TERMINAR DE EJECUTAR///////////////////////////////////

void terminar_ejecucion_memoria()
{
    log_warning(logger, "[MEMORIA]: Finalizando ejecucion...");
    
    free(espacio_usuario);
    log_info(logger,"LIBERADO ESPACIO USUARIO");
    //free(segmento_compartido);
    //log_info(logger,"LIBERADO SEGMENTO COMPARTIDO"); LO HACE LIBERAR TABLA DE SEGMENTOS GLOBALES

    list_destroy_and_destroy_elements(huecos_libres,(void*)free);
    log_info(logger,"LIBERADO HUECOS LIBRES");
    list_destroy_and_destroy_elements(procesos_globales,(void*)free);
    log_info(logger,"LIBERADO PROCESOS GLOBALES");
    list_destroy_and_destroy_elements(tabla_de_segmentos_globales,(void*)free);
    log_info(logger,"LIBERADO TABLA DE SEGMENTOS GLOBALES");

    log_destroy(logger);
    config_destroy(config);
}