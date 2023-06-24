#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]

t_queue *cola_ready;
t_queue *cola_io;

sem_t semaforo_planificador;
sem_t semaforo_ejecutando;
sem_t semaforo_ready;

sem_t semaforo_multiprogramacion;

sem_t semaforo_new;
sem_t semaforo_io;

pthread_mutex_t mx_procesos;

t_list *procesos;
t_list *recursos;

t_list *listaReady;

void manejar_paquete_cpu();
void manejar_hilo_io();
void manejar_hilo_ejecutar();
void manejar_hilo_memoria();

void cambiar_estado(Proceso *proceso, ESTADO estado);
void actualizar_pcb(Proceso *proceso, PCB *pcb);
Instruccion *buscar_instruccion_por_counter(Proceso *proceso, PCB *pcb);
t_queue *calcular_lista_ready_HRRN(t_queue *cola_ready);

void manejar_io(Proceso *proceso, int32_t PID, int tiempo);
void manejar_wait(Proceso *proceso, char *nombre_recurso);
void manejar_signal(Proceso *proceso, char *nombre_recurso);
void manejar_yield(Proceso *proceso, PCB *pcb);
void manejar_exit(Proceso *proceso, PCB *pcb);
void manejar_create_segment(Proceso* proceso, int32_t id_segmento, int32_t tamanio_segmento);
void manejar_delete_segment(int32_t pid, int32_t id_segmento);
void imprimir_cola(t_queue cola);

void quitar_salto_de_linea(char *cadena);
CODIGO_INSTRUCCION obtener_codigo_instruccion_numero(char *instruccion);

void avisar_a_consola_fin_proceso(Proceso *proceso);
void avisar_a_memoria_fin_proceso(Proceso *proceso);
void finalizar_proceso(Proceso* proceso, char* motivo);

void enviar_proceso_a_memoria(Proceso* proceso);

void liberar_proceso(Proceso *proceso);
void liberar_instruccion(Instruccion *instruccion);

void imprimir_lista(t_list* lista);

void actualizar_base_del_segmento_en_proceso(Proceso* proceso, int id_segmento, int base);
void imprimir_segmentos(Proceso* proceso);

int main(int argc, char **argv)
{

    // TODO: Destruir la lista al final.
    procesos = list_create();
    cola_ready = queue_create();
    cola_io = queue_create();

    sem_init(&semaforo_new, 0, 0);
    pthread_mutex_init(&mx_procesos, NULL);
    sem_init(&semaforo_io, 0, 0);

    char *path_config = argv[1];

    iniciar_logger_kernel();

    log_info(logger, "----- INICIA KERNEL -----");

    if (iniciar_config_kernel(path_config) == FAILURE)
        return EXIT_FAILURE;

    recursos = crear_recursos(KernelConfig.RECURSOS, KernelConfig.INSTANCIAS_RECURSOS);

    if (iniciar_servidor_kernel() == SUCCESS)
    {
        if (conectar_con_memoria() == FAILURE)
            return EXIT_FAILURE;

        if (conectar_con_file_system() == FAILURE)
            return EXIT_FAILURE;

        if (conectar_con_cpu() == FAILURE)
            return EXIT_FAILURE;

        conectar_con_consola();
    }

    Hilo hilo_cpu;
    pthread_create(&hilo_cpu, NULL, (void *)manejar_paquete_cpu, NULL);
    pthread_detach(hilo_cpu);

    Hilo hilo_io;
    pthread_create(&hilo_io, NULL, (void *)manejar_hilo_io, NULL);
    pthread_detach(hilo_io);

    Hilo hilo_ejecutar;
    pthread_create(&hilo_ejecutar, NULL, (void *)manejar_hilo_ejecutar, NULL);
    pthread_detach(hilo_ejecutar);

    Hilo hilo_memoria;
    pthread_create(&hilo_memoria, NULL, (void *)manejar_hilo_memoria, NULL);
    pthread_detach(hilo_memoria);

    // manejar_proceso_consola();

    sem_init(&semaforo_planificador, 0, 1);

    sem_init(&semaforo_ejecutando, 0, 1);

    sem_init(&semaforo_ready, 0, 0);

    // grado multiprogramacion
    sem_init(&semaforo_multiprogramacion, 0, atoi(KernelConfig.GRADO_MAX_MULTIPROGRAMACION));

    sem_wait(&semaforo_new); // Para que no empiece sin que no haya ningun proceso

    sleep(8);

    while (true)
    {

        sem_wait(&semaforo_planificador);

        bool en_new(Proceso * proceso)
        {
            return proceso->estado == NEW;
        }

        pthread_mutex_lock(&mx_procesos);
        t_list *procesos_en_new = list_filter(procesos, (void *)en_new);

        log_info(logger, "En NEW: %d", list_size(procesos_en_new));

        if (list_size(procesos_en_new) > 0)
        {
            sem_wait(&semaforo_multiprogramacion);

            Proceso *proceso_para_ready = (Proceso *)list_get(procesos_en_new, 0);

            cambiar_estado(proceso_para_ready, READY);
            
            enviar_proceso_a_memoria(proceso_para_ready);
  

            proceso_para_ready->pcb->cronometro_ready = temporal_create();

            queue_push(cola_ready, (Proceso *)proceso_para_ready);
            imprimir_cola(*cola_ready);

            sem_post(&semaforo_ready);

            sem_post(&semaforo_planificador);

            sleep(1);
        }
        pthread_mutex_unlock(&mx_procesos);
    }

    terminar_ejecucion();

    return EXIT_SUCCESS;
}

void manejar_paquete_cpu()
{
    while (true)
    {
        char *mensaje;
        switch (obtener_codigo_operacion(socket_cpu))
        {
        case MENSAJE:
            mensaje = obtener_mensaje_del_cliente(socket_cpu);
            log_info(logger, "[KERNEL]: Mensaje recibido de CPU: %s", mensaje);
            free(mensaje);
            break;
        case DESCONEXION:
            log_warning(logger, "[KERNEL]: Conexión de CPU terminada.");
            return;

        case PAQUETE_CPU:
            BUFFER *buffer = recibir_buffer(socket_cpu);

            PCB *pcb = malloc(sizeof(PCB));

            // Sumamos sizeof(int32_t) porque en PAQUETE se manda SIEMPRE [tamaño-valor]
            memcpy(&(pcb->PID), buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
            memcpy(&(pcb->program_counter), buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

            memcpy(&(pcb->registros_cpu), buffer->stream + sizeof(int32_t), sizeof(Registro_CPU));
            buffer->stream += (sizeof(Registro_CPU) + sizeof(int32_t));


            log_info(logger, "[KERNEL]: Llego PCB <%d>", pcb->PID);

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

            actualizar_pcb(proceso, pcb);

            Instruccion *instruccion = buscar_instruccion_por_counter(proceso, pcb);

            // Para mandar a memoria o file system?
            PAQUETE *paquete_instruccion = crear_paquete(INSTRUCCION);

            switch (obtener_codigo_instruccion_numero(instruccion->nombreInstruccion))
            {
            case IO:
                // EJEMPLO PARA OBTENER UN DATO QUE VIENE DE CPU DESPUES DE PID Y PROGRAM COUNTER
                // En IO no hace falta porque ya tenemos el tiempo guardado, sirve como ejemplo
                // int32_t tiempo = 0;
                // memcpy(&(tiempo), buffer->stream+sizeof(int32_t), sizeof(int32_t));
                // buffer->stream += (sizeof(int32_t)*2);
                log_info(logger, "[KERNEL] Llego Instruccion IO");
                manejar_io(proceso, pcb->PID, instruccion->tiempo);
                break;

            case WAIT:
                log_info(logger, "[KERNEL] Llego Instruccion WAIT  - %d", proceso->pcb->PID);
                manejar_wait(proceso, instruccion->recurso);
                break;

            case SIGNAL:
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL - %d", proceso->pcb->PID);
                manejar_signal(proceso, instruccion->recurso);
                break;

            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");

                memcpy(&(pcb->registros_cpu), buffer->stream + sizeof(int32_t), sizeof(Registro_CPU));
                buffer->stream += (sizeof(int32_t) + sizeof(Registro_CPU));

                // sleep(5);

                manejar_yield(proceso, pcb);
                break;

            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");

                memcpy(&(pcb->registros_cpu), buffer->stream + sizeof(int32_t), sizeof(Registro_CPU));
                buffer->stream += (sizeof(int32_t) + sizeof(Registro_CPU));

                manejar_exit(proceso, pcb);
                break;

            case F_OPEN:
                log_info(logger, "[KERNEL] Llego Instruccion F_OPEN - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, instruccion->nombreArchivo);

                // HACER

                // CODIGO_INSTRUCCION* f_open = F_OPEN;
                // agregar_a_paquete(paquete_instruccion,&f_open,sizeof(CODIGO_INSTRUCCION));
                // enviar_paquete_a_servidor(paquete, socket_file_system);
                // char *mensaje = obtener_mensaje_del_servidor(socket_file_system);
                // log_info(logger, "KERNEL: Recibi un mensaje de FS con motivo de F_OPEN:%s", mensaje);
                //
                break;

            case F_CLOSE:
                log_info(logger, "[KERNEL] Llego Instruccion F_CLOSE - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, instruccion->nombreArchivo);
                // HACER
                //
                break;

            case F_SEEK:
                log_info(logger, "[KERNEL] Llego Instruccion F_SEEK");
                log_info(logger, "Parametros: %s - %d", instruccion->nombreArchivo, instruccion->posicion);
                // HACER
                //
                break;

            case F_READ:
                log_info(logger, "[KERNEL] Llego Instruccion F_READ");

                // Direccion fisica la manda CPU
                memcpy(&(instruccion->direccionFisica), buffer->stream + sizeof(int32_t), sizeof(int32_t));
                buffer->stream += (sizeof(int32_t) * 2);

                log_info(logger, "Parametros: %s - %d - %d", instruccion->nombreArchivo, instruccion->direccionFisica, instruccion->cantBytes);
                // HACER
                //
                break;

            case F_WRITE:
                log_info(logger, "[KERNEL] Llego Instruccion F_WRITE");

                // Direccion fisica la manda CPU
                memcpy(&(instruccion->direccionFisica), buffer->stream + sizeof(int32_t), sizeof(int32_t));
                buffer->stream += (sizeof(int32_t) * 2);

                log_info(logger, "Parametros: %s - %d - %d", instruccion->nombreArchivo, instruccion->direccionFisica, instruccion->cantBytes);
                // HACER
                //
                break;

            case F_TRUNCATE:
                log_info(logger, "[KERNEL] Llego Instruccion F_TRUNCATE");
                log_info(logger, "Parametros: %s - %d", instruccion->nombreArchivo, instruccion->tamanioArchivo);
                // HACER
                //
                break;

            case CREATE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion CREATE_SEGMENT");
                log_info(logger, "Parametros: %d - %d", instruccion->idSegmento, instruccion->tamanioSegmento);
                log_warning(logger, "PID: <%d> - Crear Segmento - Id: <%d> - Tamaño:<%d>",
                            pcb->PID,
                            instruccion->idSegmento,
                            instruccion->tamanioSegmento);
                manejar_create_segment(proceso, instruccion->idSegmento, instruccion->tamanioSegmento);
                break;

            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                log_info(logger, "Parametros: %d", instruccion->idSegmento);
                log_warning(logger, "PID: <%d> - Eliminar Segmento - Id Segmento: <%d>",
                            pcb->PID,
                            instruccion->idSegmento);
                manejar_delete_segment(pcb->PID, instruccion->idSegmento);
                break;

            default:
                log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
                break;
            }

            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            break;
        }
    }
}

Proceso *obtener_proceso_por_pid(int32_t PID)
{
    bool comparar_pcb_ids(Proceso * proceso)
    {
        return proceso->pcb->PID == PID;
    }
    Proceso *proceso = list_find(procesos, (void *)comparar_pcb_ids);
    return proceso;
}

double calcular_response_ratio(PCB *pcb)
{

    int64_t tiempo_esperado_ready = temporal_gettime(pcb->cronometro_ready);

    // log_info(logger, "Tiempo en ready: %ld seg", tiempo_esperado_ready);
    // log_info(logger, "Tiempo en cpu : %f seg", pcb->estimacion_cpu_proxima_rafaga);

    double resultado2 = ((tiempo_esperado_ready + pcb->estimacion_cpu_proxima_rafaga) / pcb->estimacion_cpu_proxima_rafaga);

    // log_info(logger, "Response Ratio: %.2f", resultado2);

    return resultado2;
}

double calcular_estimacion_cpu(PCB *pcb)
{

    // log_info(logger, "Estimacion Anterior : %f", pcb->estimacion_cpu_anterior);
    // log_info(logger, "CPU  Real  Anterior : %ld", pcb->tiempo_cpu_real);

    double resultado = pcb->estimacion_cpu_anterior * atof((KernelConfig.HRRN_ALFA)) + pcb->tiempo_cpu_real * (1 - atof(KernelConfig.HRRN_ALFA));

    // log_info(logger, "Estimacion Nueva: %f", resultado);

    return resultado;
}

void actualizar_pcb(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos); // proceso* esta en lista compartida procesos
    proceso->pcb->program_counter = pcb->program_counter;
    proceso->pcb->registros_cpu = pcb->registros_cpu;
    proceso->pcb->tiempo_cpu_real = temporal_gettime(proceso->pcb->cronometro_exec);
    proceso->pcb->estimacion_cpu_anterior = proceso->pcb->estimacion_cpu_proxima_rafaga;
    pthread_mutex_unlock(&mx_procesos);
}

/* void actualizar_registros(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos); // proceso* esta en lista compartida procesos
    proceso->pcb->registros_cpu = pcb->registros_cpu;
    pthread_mutex_unlock(&mx_procesos);
} */

t_queue *calcular_lista_ready_HRRN(t_queue *cola_ready)
{

    Proceso *proceso = malloc(sizeof(Proceso));
    listaReady = list_create();

    while (!queue_is_empty(cola_ready))
    {

        proceso = queue_pop(cola_ready);

        // log_info(logger,"-------------------Proceso ID : %d----------------", proceso->pcb->PID);
        if (proceso->pcb->estimacion_cpu_anterior != 0)
        {

            proceso->pcb->estimacion_cpu_proxima_rafaga = calcular_estimacion_cpu(proceso->pcb);
        }

        proceso->pcb->response_Ratio = calcular_response_ratio(proceso->pcb);

        list_add(listaReady, proceso);
    }

    bool mayor_rr(Proceso * proceso1, Proceso * proceso2)
    {
        return proceso1->pcb->response_Ratio > proceso2->pcb->response_Ratio;
    }

    list_sort(listaReady, (void *)mayor_rr);

    for (int i = 0; i < list_size(listaReady); i++)
    {
        queue_push(cola_ready, list_get(listaReady, i));
    }

    list_destroy(listaReady);

    return cola_ready;
}

void imprimir_cola(t_queue cola)
{

    t_queue *copia = queue_create();
    Proceso *paraImprimir;
    int elementos = queue_size(&cola);

    char *lista_pids = string_new();
    string_append(&lista_pids, "[ ");

    for (int i = 0; i < elementos; i++)
    {

        paraImprimir = (Proceso *)queue_pop(&cola);
        string_append_with_format(&lista_pids, " %s ", string_itoa(paraImprimir->pcb->PID));
        // log_info(logger, "Proceso ID : %d ", paraImprimir->pcb->PID);
        queue_push(copia, (Proceso *)paraImprimir);
    }

    string_append(&lista_pids, " ]");

    log_info(logger, "Cola Ready %s: %s ", KernelConfig.ALGORITMO_PLANIFICACION, lista_pids);

    cola_ready = copia;

    free(lista_pids);
}

void manejar_hilo_ejecutar()
{

    sem_wait(&semaforo_ready);

    log_info(logger, "----------ENTRA A EJECUTAR-------");

    while (true)
    {

        sem_wait(&semaforo_ejecutando); // Ejecuta uno a la vez

        log_info(logger, "En ready: %d", queue_size(cola_ready));

        if (!queue_is_empty(cola_ready))
        {

            if (strcmp(KernelConfig.ALGORITMO_PLANIFICACION, "HRRN") == 0)
            {

                log_info(logger, "---------------------HRRN-------------------------");
                // log_info(logger, "Antes de Ordenar por HRRN");
                imprimir_cola(*cola_ready);

                calcular_lista_ready_HRRN(cola_ready);

                // log_info(logger,"Despues de Ordenar por HRRN");
                imprimir_cola(*cola_ready);

                log_info(logger, "---------------------HRRN-------------------------");
            }

            Proceso *proceso_a_ejecutar = (Proceso *)queue_pop(cola_ready);
            cambiar_estado(proceso_a_ejecutar, EXEC);

            // destrabar ready
            sem_post(&semaforo_planificador);

            //   Enviar PCB a CPU
            enviar_pcb_a_cpu(proceso_a_ejecutar->pcb);
            proceso_a_ejecutar->pcb->cronometro_exec = temporal_create();
        }
        else
        {
            // sem_post(&semaforo_ejecutando);
        }
    }
}

// duda por lo que dice el enunciado
// asi esta bien o 1 hilo por proceso y que sleep de ese hilo?
void manejar_hilo_io()
{
    while (true)
    {
        sem_wait(&semaforo_io);
        Proceso_IO *proceso_io = (Proceso_IO *)queue_pop(cola_io);
        log_info(logger, "[KERNEL]: PID: <%d> - Ejecuta IO: %d", proceso_io->PID, proceso_io->tiempo_bloqueado);
        sleep(proceso_io->tiempo_bloqueado);

        Proceso *proceso = obtener_proceso_por_pid(proceso_io->PID);

        log_info(logger, "[KERNEL] Poner <%d> en READY por IO", proceso_io->PID);
        cambiar_estado(proceso, READY);

        proceso->pcb->cronometro_ready = temporal_create();

        sem_post(&semaforo_planificador);
        
        queue_push(cola_ready, proceso);
        imprimir_cola(*cola_ready);

        sem_post(&semaforo_ejecutando);
    }
}

void manejar_wait(Proceso *proceso, char *nombre_recurso)
{
    // quitar_salto_de_linea(nombre_recurso);

    bool comparar_recurso_por_nombre(Recurso * recurso)
    {
        return strcmp(recurso->nombre, nombre_recurso) == 0;
    }

    Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);

    if (recurso == NULL)
    {
        log_error(logger, "[KERNEL]: PID: <%d> - FINALIZADO POR ERROR - WAIT RECURSO NO EXISTENTE (%s)", proceso->pcb->PID, nombre_recurso);
        //cambiar_estado(proceso, FINISHED);
        //sem_post(&semaforo_multiprogramacion);

        finalizar_proceso(proceso, "INVALID_RESOURCE");
        return;
    }

    if (recurso->instancias > 0)
    {
        recurso->instancias -= 1;
        list_add(proceso->pcb->recursos_asignados, nombre_recurso);
        

        log_info(logger, "[KERNEL]: PID: <%d> - WAIT: %s - INSTANCIAS: %d", proceso->pcb->PID, nombre_recurso, recurso->instancias);

        proceso->pcb->program_counter++;
        cambiar_estado(proceso, READY);
        proceso->pcb->cronometro_ready = temporal_create();
        queue_push(cola_ready, proceso);
        imprimir_cola(*cola_ready);
    }
    else
    {
        cambiar_estado(proceso, BLOCK);

        log_error(logger, "[KERNEL] PID: <%d> - Bloqueado por: %s", proceso->pcb->PID, nombre_recurso);

        queue_push(recurso->cola_block, proceso);
    }
}

void manejar_signal(Proceso *proceso, char *nombre_recurso)
{
    // quitar_salto_de_linea(nombre_recurso);
    bool comparar_recurso_por_nombre(Recurso * recurso)
    {
        return strcmp(recurso->nombre, nombre_recurso) == 0;
    }

    Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);

    if (recurso == NULL)
    {
        log_error(logger, "[KERNEL]: PID: <%d> - FINALIZADO POR ERROR - SIGNAL DE RECURSO NO EXISTENTE (%s)", proceso->pcb->PID, nombre_recurso);
        //cambiar_estado(proceso, FINISHED);
        //sem_post(&semaforo_multiprogramacion);

        finalizar_proceso(proceso, "INVALID_RESOURCE");
        return;
    }

    recurso->instancias += 1;
    list_remove(proceso->pcb->recursos_asignados, nombre_recurso);

    log_info(logger, "[KERNEL]: PID: <%d> - SIGNAL: %s - INSTANCIAS: %d", proceso->pcb->PID, nombre_recurso, recurso->instancias);

    if (!queue_is_empty(recurso->cola_block))
    {
        log_info(logger, "[KERNEL] desbloquear %s ", nombre_recurso);
        Proceso *proceso_bloqueado = (Proceso *)queue_pop(recurso->cola_block);
        // estado es EXEC? hay que sacarlo de block
        proceso_bloqueado->estado = EXEC;
        //
        list_add(proceso->pcb->recursos_asignados, nombre_recurso);
    }

    proceso->pcb->program_counter++;
    cambiar_estado(proceso, READY);
    proceso->pcb->cronometro_ready = temporal_create();
    queue_push(cola_ready, proceso);
    imprimir_cola(*cola_ready);
}

Instruccion *buscar_instruccion_por_counter(Proceso *proceso, PCB *pcb)
{
    Instruccion *instruccion = list_get(proceso->pcb->instrucciones, pcb->program_counter);

    return instruccion;
}

void manejar_yield(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos);

    proceso->pcb->program_counter++;

    proceso->pcb->registros_cpu = pcb->registros_cpu;

    log_info(logger, "[KERNEL] Poner <%d> en READY por YIELD", proceso->pcb->PID);
    cambiar_estado(proceso, READY);

    proceso->pcb->cronometro_ready = temporal_create();
    queue_push(cola_ready, proceso);
    imprimir_cola(*cola_ready);

    pthread_mutex_unlock(&mx_procesos);
}

void manejar_exit(Proceso *proceso, PCB *pcb)
{
    proceso->pcb->registros_cpu = pcb->registros_cpu;

    //cambiar_estado(proceso, FINISHED);
    //sem_post(&semaforo_multiprogramacion);

    finalizar_proceso(proceso, "SUCCESS");
}

void finalizar_proceso(Proceso* proceso, char* motivo){
    log_warning(logger, "[KERNEL]: PID: <%d> - FINALIZADO - Motivo: <%s>", proceso->pcb->PID, motivo);

    cambiar_estado(proceso, FINISHED);
    sem_post(&semaforo_multiprogramacion);

    // avisar a memoria para que libere estructuras
    avisar_a_memoria_fin_proceso(proceso);
    // avisar a consola que finalizo
    avisar_a_consola_fin_proceso(proceso);
    //
    liberar_recursos(proceso);
    //
    liberar_proceso(proceso);
}

void manejar_io(Proceso *proceso, int32_t PID, int tiempo)
{
    cambiar_estado(proceso, BLOCK);

    log_error(logger, "[KERNEL] PID: <%d> - Bloqueado por: IO", PID);

    Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
    proceso_io->PID = PID;
    proceso_io->tiempo_bloqueado = tiempo;

    proceso->pcb->program_counter++;

    queue_push(cola_io, proceso_io);

    sem_post(&semaforo_io);
}

void avisar_a_consola_fin_proceso(Proceso *proceso)
{
    log_info(logger, "[KERNEL]: Avisando a CONSOLA que finalizo el proceso PID <%d> - SOCKET_CONSOLA: <%d>", proceso->pcb->PID, proceso->pcb->socket_consola);
    PAQUETE *paquete = crear_paquete(PROCESO_FINALIZADO);
    enviar_paquete_a_cliente(paquete, proceso->pcb->socket_consola);
}

void avisar_a_memoria_fin_proceso(Proceso *proceso)
{
    log_info(logger, "[KERNEL]: Avisando a MEMORIA que finalizo el proceso PID <%d>", proceso->pcb->PID);
    PAQUETE *paquete = crear_paquete(FINALIZAR_PROCESO);
    agregar_a_paquete(paquete, &proceso->pcb->PID, sizeof(int32_t));
    enviar_paquete_a_cliente(paquete, socket_memoria);
}

void liberar_instruccion(Instruccion *instruccion)
{
    free(instruccion->nombreInstruccion);
    free(instruccion->valor);
    free(instruccion->registro);
    free(instruccion->nombreArchivo);
    free(instruccion->recurso);
}

void liberar_proceso(Proceso *proceso)
{
    list_destroy_and_destroy_elements(proceso->pcb->instrucciones, liberar_instruccion);
    temporal_destroy(proceso->pcb->cronometro_ready);
    temporal_destroy(proceso->pcb->cronometro_exec);
    // free(proceso->pcb); -> Todavia no se puede liberar. Hacer free al final
}

void liberar_recursos(Proceso *proceso)
{
    if (!list_is_empty(proceso->pcb->recursos_asignados))
    {
        for (int i = 0; i < list_size(proceso->pcb->recursos_asignados); i++)
        {

            bool comparar_recurso_por_nombre(Recurso * recurso)
            {
                return strcmp(recurso->nombre, list_get(proceso->pcb->recursos_asignados, i)) == 0;
            }

            Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);
            recurso->instancias += 1;

            log_info(logger, "[KERNEL] SUMAMOS RECURSO %s - %d ", recurso->nombre, recurso->instancias);
        }

        list_destroy_and_destroy_elements(proceso->pcb->recursos_asignados, free);
    }
}


void manejar_hilo_memoria()
{

    int32_t PID;
    Proceso* proceso;
    int32_t base;
    int32_t id_segmento;
    t_list* tabla_segmentos;

    while (true)
    {
        switch (obtener_codigo_operacion(socket_memoria))
        {
            
        case CREAR_PROCESO:
            log_info(logger, "[KERNEL]: Llego tabla de segmentos de MEMORIA");

            BUFFER *buffer = recibir_buffer(socket_memoria);

            memcpy(&PID, buffer->stream, sizeof(int32_t));
            buffer->stream += sizeof(int32_t);

            tabla_segmentos = deserializar_segmentos(buffer);
        
            proceso = obtener_proceso_por_pid(PID);
            proceso->pcb->tabla_segmentos = tabla_segmentos;
            imprimir_segmentos(proceso);

            break;

        case CREAR_SEGMENTO:
        log_info(logger, "[KERNEL]: Se puede crear el segmento !! ");

            BUFFER *buffer_segmento = recibir_buffer(socket_memoria);

            memcpy(&PID, buffer_segmento->stream + sizeof(int32_t), sizeof(int32_t));
            buffer_segmento->stream += (sizeof(int32_t)*2);

            memcpy(&id_segmento, buffer_segmento->stream + sizeof(int32_t), sizeof(int32_t));
            buffer_segmento->stream += (sizeof(int32_t)*2);
           
            memcpy(&base, buffer_segmento->stream + sizeof(int32_t), sizeof(int32_t));
            buffer_segmento->stream += (sizeof(int32_t)*2);

            proceso = obtener_proceso_por_pid(PID);

            log_info(logger, "Proceso PID: %d  - Base: %d ", PID, base);

            actualizar_base_del_segmento_en_proceso(proceso, id_segmento, base);
            imprimir_segmentos(proceso);

            // DEVOLVER A CPU EL PROCESO
            proceso->pcb->program_counter++;
            enviar_pcb_a_cpu(proceso->pcb);
            proceso->pcb->cronometro_exec = temporal_create();

            break;

        case CONSOLIDAR:
        log_info(logger, "[KERNEL]: CONSOLIDAR para crear segmento de memoria !! ");

            BUFFER *buffer_consolidar = recibir_buffer(socket_memoria);

            memcpy(&PID, buffer_consolidar->stream + sizeof(int32_t), sizeof(int32_t));
            buffer_consolidar->stream += (sizeof(int32_t)*2);

            proceso = obtener_proceso_por_pid(PID);

            log_info(logger, "Proceso PID: %d", proceso->pcb->PID); 

            /*
                Puede consolidar ? SI --- Envio Peticion
                NO --- Espero hasta que se pueda...Envio Peticion

                Deberiamos esperar con un semaforo? 
            */

            //verificar_operaciones_FS_y_Memoria();     (F_READ y F_WRITE)

            //avisar_puede_crear_segmento();  COD OP: SOLICITAR_COMPACTACION

            break;

         case COMPACTACION_TERMINADA:
            
            log_info(logger, "[KERNEL]: Memoria Termino la consolidacion !! ");
            log_info(logger, "[KERNEL]: Recibimos TABLA DE SEGMENTOS de TODOS LOS PROCESOS ");

            //actualizar_segmentos_procesos();

            break;

        case FALTA_MEMORIA:
        log_info(logger, "[KERNEL]: FALTA MEMORIAAAAA!! Terminar el proceso con error ");

            BUFFER *buffer_fallo = recibir_buffer(socket_memoria);

            memcpy(&PID, buffer_fallo->stream + sizeof(int32_t), sizeof(int32_t));
            buffer_fallo->stream += (sizeof(int32_t)*2);

            proceso = obtener_proceso_por_pid(PID);

            log_info(logger, "Proceso PID: %d", proceso->pcb->PID);

            finalizar_proceso(proceso, "OUT_OF_MEMORY");  // Falta Manejarlo en Memoria

            break;


        case BORRAR_SEGMENTO:
            log_info(logger, "[KERNEL]: Llego BORRAR SEGMENTO de MEMORIA");

            BUFFER *buffer_borrar = recibir_buffer(socket_memoria);

            memcpy(&PID, buffer_borrar->stream, sizeof(int32_t));
            buffer_borrar->stream += sizeof(int32_t);

            tabla_segmentos = deserializar_segmentos(buffer_borrar);
        
            proceso = obtener_proceso_por_pid(PID);
            proceso->pcb->tabla_segmentos = tabla_segmentos;

            imprimir_segmentos(proceso);

            // DEVOLVER A CPU EL PROCESO
            proceso->pcb->program_counter++;
            enviar_pcb_a_cpu(proceso->pcb);
            proceso->pcb->cronometro_exec = temporal_create();
            
            break;

        case FINALIZAR_PROCESO:
            log_info(logger, "[KERNEL]: Llego FINALIZAR_PROCESO de MEMORIA");
            break;

        default:
            log_warning(logger, "[KERNEL]: Operacion desconocida desde MEMORIA.");
            break;
        }
    }
}

void actualizar_base_del_segmento_en_proceso(Proceso* proceso, int id_segmento, int base){
    bool comparar_ids_segmento(SEGMENTO* segmento)
    {
        return segmento->id == id_segmento;
    }

    SEGMENTO *segmento = list_find(proceso->pcb->tabla_segmentos, (void *)comparar_ids_segmento);

    segmento->base = base;
    log_info(logger, "[KERNEL] PID: %d - Actualizamos segmento %d con base %d", proceso->pcb->PID, segmento->id, segmento->base);
}

void enviar_proceso_a_memoria(Proceso* proceso){
    log_info(logger, "[KERNEL] Enviando a MEMORIA proceso PID <%d>", proceso->pcb->PID);
    PAQUETE *paquete = crear_paquete(CREAR_PROCESO);
    agregar_a_paquete(paquete, &proceso->pcb->PID, sizeof(int32_t));
    enviar_paquete_a_cliente(paquete, socket_memoria);
}

CODIGO_INSTRUCCION obtener_codigo_instruccion_numero(char *instruccion)
{
    // quitar_salto_de_linea(instruccion);
    if (strcmp(instruccion, "MOV_IN") == 0)
        return MOV_IN;
    else if (strcmp(instruccion, "MOV_OUT") == 0)
        return MOV_OUT;
    else if (strcmp(instruccion, "I/O") == 0)
        return IO;
    else if (strcmp(instruccion, "F_OPEN") == 0)
        return F_OPEN;
    else if (strcmp(instruccion, "F_CLOSE") == 0)
        return F_CLOSE;
    else if (strcmp(instruccion, "F_SEEK") == 0)
        return F_SEEK;
    else if (strcmp(instruccion, "F_READ") == 0)
        return F_READ;
    else if (strcmp(instruccion, "F_WRITE") == 0)
        return F_WRITE;
    else if (strcmp(instruccion, "F_TRUNCATE") == 0)
        return F_TRUNCATE;
    else if (strcmp(instruccion, "WAIT") == 0)
        return WAIT;
    else if (strcmp(instruccion, "SIGNAL") == 0)
        return SIGNAL;
    else if (strcmp(instruccion, "CREATE_SEGMENT") == 0)
        return CREATE_SEGMENT;
    else if (strcmp(instruccion, "DELETE_SEGMENT") == 0)
        return DELETE_SEGMENT;
    else if (strcmp(instruccion, "YIELD") == 0)
        return YIELD;
    else if (strcmp(instruccion, "EXIT") == 0)
        return EXIT;

    return EXIT;
}

char *descripcion_estado(ESTADO estado)
{
    switch (estado)
    {
    case NEW:
        return "NEW";
    case READY:
        return "READY";
    case EXEC:
        return "EXEC";
    case BLOCK:
        return "BLOCK";
    case FINISHED:
        return "FINISHED";
    }
}

void cambiar_estado(Proceso *proceso, ESTADO estado)
{
    ESTADO anterior = proceso->estado;
    proceso->estado = estado;
    log_warning(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s> - Estado Actual: <%s>", proceso->pcb->PID, descripcion_estado(anterior), descripcion_estado(proceso->estado));
}

void manejar_create_segment(Proceso* proceso, int32_t id_segmento, int32_t tamanio_segmento)
{
    SEGMENTO *segmento = malloc(sizeof(SEGMENTO));
    segmento->id = id_segmento;
    segmento->base = 0;
    segmento->limite = 0;
    segmento->validez = 0;

    list_add(proceso->pcb->tabla_segmentos, segmento);

    PAQUETE *paquete = crear_paquete(INSTRUCCION);
    int32_t cs = CREATE_SEGMENT;
    agregar_a_paquete(paquete, &cs, sizeof(int32_t));
    agregar_a_paquete(paquete, &proceso->pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete, &segmento->id, sizeof(int32_t));
    agregar_a_paquete(paquete, &tamanio_segmento, sizeof(int32_t));
    enviar_paquete_a_servidor(paquete, socket_memoria);
    eliminar_paquete(paquete);

    log_info(logger, "ENVIÉ PAQUETE A MEMORIA CON MOTIVO: <CREATE_SEGMENT> - id_segmento: %d - tamanio: %d", segmento->id, tamanio_segmento);

    // RECIBIR RTA ... TODO Manejado en HILO MEMORIA 

}

void manejar_delete_segment(int32_t pid, int32_t id_segmento)
{
    PAQUETE *paquete = crear_paquete(INSTRUCCION);
    int32_t ds = DELETE_SEGMENT;
    agregar_a_paquete(paquete, &ds, sizeof(int32_t));
    agregar_a_paquete(paquete, &pid, sizeof(int32_t));
    agregar_a_paquete(paquete, &id_segmento, sizeof(int32_t));
    enviar_paquete_a_servidor(paquete, socket_memoria);
    eliminar_paquete(paquete);
    log_info(logger, "ENVIÉ PAQUETE A MEMORIA CON MOTIVO: <DELETE_SEGMENT>");

    // RECIBIR RTA ... TODO Manejado en HILO MEMORIA

    //respuesta de la Memoria la tabla de segmentos actualizada.
}

void imprimir_lista(t_list* lista){
    for(int i = 0; i < list_size(lista); i++){
        char* el = list_get(lista, i) + '\0';
        log_info(logger, "%s ; ", el);
    }
}

void imprimir_segmentos(Proceso* proceso){
    log_info(logger, "-----------------");
    log_info(logger, "PID: %d - SEGMENTOS", proceso->pcb->PID);
    for(int i = 0; i < list_size(proceso->pcb->tabla_segmentos); i++){
        SEGMENTO* segmento = (SEGMENTO*) list_get(proceso->pcb->tabla_segmentos, i);
        log_info(logger, "-> ID %d - Base %d - Limite %d - Validez %d", segmento->id, segmento->base, segmento->limite, segmento->validez);
    }
    log_info(logger, "-----------------");
}