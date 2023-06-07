#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]

t_queue *cola_ready;
t_queue *cola_io;

sem_t semaforo_planificador;
sem_t semaforo_ejecutando;
sem_t semaforo_new;
sem_t semaforo_io;

pthread_mutex_t mx_procesos;

t_list *procesos;
t_list *recursos;

void manejar_paquete_cpu();
void manejar_io();

void cambiar_estado(Proceso *proceso, ESTADO estado);
void actualizar_pcb(Proceso *proceso, PCB *pcb);
Instruccion *buscar_instruccion_por_counter(Proceso *proceso, PCB *pcb);

void manejar_wait(Proceso *proceso, char *nombre_recurso);
void manejar_signal(Proceso *proceso, char *nombre_recurso);
void manejar_yield(Proceso *proceso);
void manejar_exit(Proceso *proceso);

CODIGO_INSTRUCCION obtener_codigo_instruccion_numero(char *instruccion);

int main()
{

    // TODO: Destruir la lista al final.
    procesos = list_create();
    cola_ready = queue_create();
    cola_io = queue_create();

    sem_init(&semaforo_new, 0, 0);
    pthread_mutex_init(&mx_procesos, NULL);
    sem_init(&semaforo_io, 0, 0);

    iniciar_logger_kernel();

    iniciar_config_kernel();

    recursos = crear_recursos(KernelConfig.RECURSOS, KernelConfig.INSTANCIAS_RECURSOS);

    if (iniciar_servidor_kernel() == SUCCESS)
    {
        conectar_con_memoria();

        conectar_con_file_system();

        conectar_con_cpu();

        conectar_con_consola();
    }

    Hilo hilo_cpu;
    pthread_create(&hilo_cpu, NULL, (void *)manejar_paquete_cpu, NULL);
    pthread_detach(hilo_cpu);

    Hilo hilo_io;
    pthread_create(&hilo_io, NULL, (void *)manejar_io, NULL);
    pthread_detach(hilo_io);

    // manejar_proceso_consola();

    // grado multiprogramacion
    sem_init(&semaforo_planificador, 0, 1);

    sem_init(&semaforo_ejecutando, 0, 1);

    sem_wait(&semaforo_new); // Para que no empiece sin que no haya ningun proceso

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
            Proceso *proceso_para_ready = (Proceso *)list_get(procesos_en_new, 0);
            proceso_para_ready->estado = READY;
            queue_push(cola_ready, (Proceso *)proceso_para_ready);
            log_info(logger, "Proceso %d -> READY", (proceso_para_ready->pcb)->PID);
        }
        pthread_mutex_unlock(&mx_procesos);

        sem_wait(&semaforo_ejecutando); // Ejecuta uno a la vez

        log_info(logger, "En ready: %d", queue_size(cola_ready));
        if (!queue_is_empty(cola_ready))
        {
            Proceso *proceso_a_ejecutar = (Proceso *)queue_pop(cola_ready);
            proceso_a_ejecutar->estado = EXEC;

            // destrabar ready
            sem_post(&semaforo_planificador);
            log_info(logger, "Proceso %d -> EXEC", (proceso_a_ejecutar->pcb)->PID);
            sleep(5);
            // Tomar el time ejecucion HRRN
            //  Enviar PCB a CPU
            enviar_pcb_a_cpu(proceso_a_ejecutar->pcb);
        }
        else
        {
            sem_post(&semaforo_ejecutando);
        }
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
            log_info(logger, "[KERNEL] Llego PCB 2");

            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            BUFFER *buffer = recibir_buffer(socket_cpu);

            PCB *pcb = malloc(sizeof(PCB));

            // Sumamos sizeof(int32_t) porque en PAQUETE se manda SIEMPRE [tamaño-valor]
            memcpy(&(pcb->PID), buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
            memcpy(&(pcb->program_counter), buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

            actualizar_pcb(proceso, pcb);

            Instruccion *instruccion = buscar_instruccion_por_counter(proceso, pcb);

            // Para mandar a memoria o file system?
            PAQUETE *paquete_instruccion = crear_paquete(INSTRUCCION);

            switch (obtener_codigo_instruccion_numero(instruccion->nombreInstruccion))
            {
            case IO:
                log_info(logger, "[KERNEL] Llego Instruccion IO");

                // EJEMPLO PARA OBTENER UN DATO QUE VIENE DE CPU DESPUES DE PID Y PROGRAM COUNTER
                // En IO no hace falta porque ya tenemos el tiempo guardado, sirve como ejemplo
                // int32_t tiempo = 0;
                // memcpy(&(tiempo), buffer->stream+sizeof(int32_t), sizeof(int32_t));
                // buffer->stream += (sizeof(int32_t)*2);

                cambiar_estado(proceso, BLOCK);

                Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
                proceso_io->PID = pcb->PID;
                proceso_io->tiempo_bloqueado = instruccion->tiempo;

                queue_push(cola_io, proceso_io);

                sem_post(&semaforo_io);
                break;

            case WAIT:
                log_info(logger, "[KERNEL] Llego Instruccion WAIT");
                manejar_wait(proceso, instruccion->recurso);
                break;

            case SIGNAL:
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL");
                manejar_signal(proceso, instruccion->recurso);
                break;

            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");
                manejar_yield(proceso);
                break;

            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");
                manejar_exit(proceso);
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
                // HACER
                //
                break;

            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                log_info(logger, "Parametros: %d", instruccion->idSegmento);
                // HACER
                //
                break;

            default:
                log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
                break;
            }

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

void actualizar_pcb(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos); // proceso* esta en lista compartida procesos
    // Proceso *proceso = obtener_proceso_por_pid(pcb->PID);
    proceso->pcb->program_counter = pcb->program_counter;
    // algun dato mas?
    pthread_mutex_unlock(&mx_procesos);
}

// duda por lo que dice el enunciado
// asi esta bien o 1 hilo por proceso y que sleep de ese hilo?
void manejar_io()
{
    while (true)
    {
        sem_wait(&semaforo_io);
        Proceso_IO *proceso_io = (Proceso_IO *)queue_pop(cola_io);
        sleep(proceso_io->tiempo_bloqueado);

        Proceso *proceso = obtener_proceso_por_pid(proceso_io->PID);

        cambiar_estado(proceso, READY);

        queue_push(cola_ready, proceso);
    }
}

void manejar_wait(Proceso *proceso, char *nombre_recurso)
{
    bool comparar_recurso_por_nombre(Recurso * recurso)
    {
        return strcmp(recurso->nombre, nombre_recurso) == 0;
    }

    Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);

    if (recurso == NULL)
    {
        cambiar_estado(proceso, EXIT);
        return;
    }

    if (recurso->instancias > 0)
    {
        recurso->instancias -= 1;
    }
    else
    {
        cambiar_estado(proceso, BLOCK);

        queue_push(recurso->cola_block, proceso);
    }
}

void manejar_signal(Proceso *proceso, char *nombre_recurso)
{
    bool comparar_recurso_por_nombre(Recurso * recurso)
    {
        return strcmp(recurso->nombre, nombre_recurso) == 0;
    }

    Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);

    if (recurso == NULL)
    {
        cambiar_estado(proceso, EXIT);
        return;
    }

    recurso->instancias += 1;

    if (!queue_is_empty(recurso->cola_block))
    {
        Proceso *proceso_bloqueado = (Proceso *)queue_pop(recurso->cola_block);
        // estado es EXEC? hay que sacarlo de block
        proceso_bloqueado->estado = EXEC;
    }
}

Instruccion *buscar_instruccion_por_counter(Proceso *proceso, PCB *pcb)
{
    Instruccion *instruccion = list_get(proceso->pcb->instrucciones, pcb->program_counter);

    return instruccion;
}

void manejar_yield(Proceso *proceso)
{
    pthread_mutex_lock(&mx_procesos);

    cambiar_estado(proceso, READY);
    queue_push(procesos, proceso);

    pthread_mutex_unlock(&mx_procesos);
}

void manejar_exit(Proceso *proceso)
{
    cambiar_estado(proceso, EXIT);
    // liberar recursos
}

CODIGO_INSTRUCCION obtener_codigo_instruccion_numero(char *instruccion)
{
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
    log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s> - Estado Actual: <%s>", proceso->pcb->PID, descripcion_estado(anterior), descripcion_estado(proceso->estado));
}