#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]
// cola_ready: [1, 2, 3]
// cola_bloqueados: [5]

t_queue *cola_ready;
t_queue *cola_io;

sem_t semaforo_planificador;

sem_t semaforo_ejecutando;

sem_t semaforo_new;

sem_t semaforo_io;

pthread_mutex_t mx_procesos;

t_list *procesos;

void manejar_paquete_cpu();

int main()
{

    // TODO: Destruir la lista al final.
    procesos = list_create();
    cola_ready = queue_create();

    sem_init(&semaforo_new, 0, 0);
    pthread_mutex_init(&mx_procesos, NULL);
    sem_init(&semaforo_io, 0, 0);

    iniciar_logger_kernel();

    iniciar_config_kernel();

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

    sleep(5);

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

            // Enviar PCB a CPU
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

        case OP_PCB:
            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            PCB *pcb = obtener_paquete_pcb(socket_cpu);
            log_info(logger, "[KERNEL] Llego PCB %d de CPU", pcb->PID);

            reemplazar_pcb_en_procesos(pcb);

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

            Lista *lista_parametros = obtener_paquete_como_lista(socket_cpu);
            CODIGO_INSTRUCCION codigo_instruccion = *(int32_t *)list_get(lista_parametros, 0);

            switch (codigo_instruccion)
            {
            case IO:
                log_info(logger, "[KERNEL] Llego Instruccion IO");
                int tiempo_io = *(int32_t *)list_get(lista_parametros, 1);
                //
                proceso->estado = BLOCK;

                Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
                proceso_io->PID = pcb->PID;
                proceso_io->tiempo_bloqueado = tiempo_io;

                queue_push(cola_io, proceso_io);

                sem_post(&semaforo_io);
                //
                break;
            case F_OPEN:
                log_info(logger, "[KERNEL] Llego Instruccion F_OPEN");
                char *nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                //
                break;
            case F_CLOSE:
                log_info(logger, "[KERNEL] Llego Instruccion F_CLOSE");
                char *nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                //
                break;
            case F_SEEK:
                log_info(logger, "[KERNEL] Llego Instruccion F_SEEK");
                char *nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int posicion = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case F_READ:
                log_info(logger, "[KERNEL] Llego Instruccion F_READ");
                char *nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                int cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_WRITE:
                log_info(logger, "[KERNEL] Llego Instruccion F_WRITE");
                char *nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                int cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_TRUNCATE:
                log_info(logger, "[KERNEL] Llego Instruccion F_TRUNCATE");
                char *nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int tamanio_archivo = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case WAIT:
                log_info(logger, "[KERNEL] Llego Instruccion WAIT");
                char *recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                //
                break;
            case SIGNAL:
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL");
                char *recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                //
                break;
            case CREATE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion CREATE_SEGMENT");
                int id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                int tamanio_segmento = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                int id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                //
                break;
            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");
                // No se envia nada
                //
                break;
            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");
                // No se envia nada
                //
                break;
            }

            break;
        default:
            log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
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

void reemplazar_pcb_en_procesos(PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos);
    Proceso *proceso = obtener_proceso_por_pid(pcb->PID);
    proceso->pcb = pcb;
    pthread_mutex_unlock(&mx_procesos);
}

void manejar_io()
{
    while (true)
    {
        sem_wait(&semaforo_io);
        Proceso_IO *proceso_io = (Proceso_IO *)queue_pop(cola_io);
        sleep(proceso_io->tiempo_bloqueado);

        Proceso *proceso = obtener_proceso_por_pid(pcb->PID);
        proceso->estado = READY;

        queue_push(cola_ready, proceso);
    }
}