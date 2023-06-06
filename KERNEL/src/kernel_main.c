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

void reemplazar_pcb_en_procesos(PCB *pcb);
Instruccion *buscar_instruccion_por_counter(PCB *pcb);

void manejar_wait(Proceso *proceso, char *nombre_recurso);
void manejar_signal(Proceso *proceso, char *nombre_recurso);
void manejar_yield(Proceso *proceso);
void manejar_exit(Proceso *proceso);

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

        case PAQUETE_2:
            log_info(logger, "[KERNEL] Llego PCB 2");

            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            BUFFER *buffer = recibir_buffer(socket_cpu);

            PCB *pcb = malloc(sizeof(PCB));

            memcpy(&(pcb->PID), buffer->stream+sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t)*2);
            memcpy(&(pcb->program_counter), buffer->stream+sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t)*2);         

            printf("Valor recibido para PCB->PID: %d\n", pcb->PID);
            printf("Valor recibido para PCB->program_counter: %d\n", pcb->program_counter);  

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);     

            Instruccion * instruccion = buscar_instruccion_por_counter(pcb);       

            switch (obtener_Codigo_Instruccion_numero(instruccion->nombreInstruccion))
            {
            case IO:
                log_info(logger,"Llego IO ATRRRRR!!!");

                int32_t tiempo = 0;
                memcpy(&(tiempo), buffer->stream+sizeof(int32_t), sizeof(int32_t));
                buffer->stream += (sizeof(int32_t)*2);

                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%d>", proceso->pcb->PID, proceso->estado);
                proceso->estado = BLOCK;
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%d>", proceso->pcb->PID, proceso->estado);

                Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
                proceso_io->PID = pcb->PID;
                proceso_io->tiempo_bloqueado = tiempo;

                queue_push(cola_io, proceso_io);

                sem_post(&semaforo_io);
                break;
            
            case WAIT:
                break;

            case SIGNAL:
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL");
                // recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                // manejar_signal(proceso, recurso);
                break;

            case YIELD:
                break;

            case EXIT:
                break;

            default:
                break;
            }



                break;
        case OP_PCB:
            /*log_info(logger, "[KERNEL] Llego PCB");
            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            PCB *pcb = obtener_paquete_pcb(socket_cpu);
            log_info(logger, "[KERNEL] Llego PCB %d de CPU", pcb->PID);

            reemplazar_pcb_en_procesos(pcb);

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

            Lista *lista_parametros = obtener_paquete_como_lista(socket_cpu);
            char *c = (char *)list_get(lista_parametros, 0);

            CODIGO_INSTRUCCION codigo_instruccion = *(int32_t *)list_get(lista_parametros, 0);

            char *nombre_archivo, recurso;
            int direccion_fisica, cant_bytes;
            int id_segmento, tamanio_segmento;

            PAQUETE *paquete_instruccion = crear_paquete(INSTRUCCION);

            switch (codigo_instruccion)
            {
            case IO:
                //

                int tiempo_io = *(int32_t *)list_get(lista_parametros, 1);
                log_info(logger, "[KERNEL] Llego Instruccion IO - Proceso PID:<%d> - Tiempo IO : <%d>", proceso->pcb->PID, tiempo_io);
                //
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%d>", proceso->pcb->PID, proceso->estado);
                proceso->estado = BLOCK;
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%d>", proceso->pcb->PID, proceso->estado);

                Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
                proceso_io->PID = pcb->PID;
                proceso_io->tiempo_bloqueado = tiempo_io;

                queue_push(cola_io, proceso_io);

                sem_post(&semaforo_io);
                //
                break;
            case F_OPEN:
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                log_info(logger, "[KERNEL] Llego Instruccion F_OPEN - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, nombre_archivo);

                // CODIGO_INSTRUCCION* f_open = F_OPEN;
                // agregar_a_paquete(paquete_instruccion,&f_open,sizeof(CODIGO_INSTRUCCION));
                // enviar_paquete_a_servidor(paquete, socket_file_system);
                // char *mensaje = obtener_mensaje_del_servidor(socket_file_system);
                // log_info(logger, "KERNEL: Recibi un mensaje de FS con motivo de F_OPEN:%s", mensaje);
                //
                break;
            case F_CLOSE:
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                log_info(logger, "[KERNEL] Llego Instruccion F_CLOSE - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, nombre_archivo);

                //
                break;
            case F_SEEK:
                log_info(logger, "[KERNEL] Llego Instruccion F_SEEK");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int posicion = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case F_READ:
                log_info(logger, "[KERNEL] Llego Instruccion F_READ");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_WRITE:
                log_info(logger, "[KERNEL] Llego Instruccion F_WRITE");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_TRUNCATE:
                log_info(logger, "[KERNEL] Llego Instruccion F_TRUNCATE");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int tamanio_archivo = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case WAIT:
                //
                log_info(logger, "[KERNEL] Llego Instruccion WAIT");
                recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                manejar_wait(proceso, recurso);
                //
                break;
            case SIGNAL:
                //
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL");
                recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                manejar_signal(proceso, recurso);
                //
                break;
            case CREATE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion CREATE_SEGMENT");
                id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                tamanio_segmento = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                //
                break;
            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");
                //
                manejar_yield(proceso);
                //
                break;
            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");
                //
                manejar_exit(proceso);
                //
                break;
            default:
                log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
                break;
            }
            */
        }
    }
}
/*
void manejar_paquete_cpu2()
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

        case PAQUETE_2:
               BUFFER* buffer = recibir_buffer(socket_cpu);
               PCB *pcb = deserializar_pcb(buffer);

               Instruccion  * instruccion = buscar_instruccion_por_counter(pcb);
            switch (codigo_instruccion)
            {
            case IO:
                //

                int tiempo_io = *(int32_t *)list_get(lista_parametros, 1);
                log_info(logger, "[KERNEL] Llego Instruccion IO - Proceso PID:<%d> - Tiempo IO : <%d>", proceso->pcb->PID, tiempo_io);
                //
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%d>", proceso->pcb->PID, proceso->estado);
                proceso->estado = BLOCK;
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%d>", proceso->pcb->PID, proceso->estado);

                Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
                proceso_io->PID = pcb->PID;
                proceso_io->tiempo_bloqueado = tiempo_io;

                queue_push(cola_io, proceso_io);

                sem_post(&semaforo_io);
                //
                break;
            case F_OPEN:
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                log_info(logger, "[KERNEL] Llego Instruccion F_OPEN - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, nombre_archivo);

                // CODIGO_INSTRUCCION* f_open = F_OPEN;
                // agregar_a_paquete(paquete_instruccion,&f_open,sizeof(CODIGO_INSTRUCCION));
                // enviar_paquete_a_servidor(paquete, socket_file_system);
                // char *mensaje = obtener_mensaje_del_servidor(socket_file_system);
                // log_info(logger, "KERNEL: Recibi un mensaje de FS con motivo de F_OPEN:%s", mensaje);
                //
                break;
            case F_CLOSE:
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                log_info(logger, "[KERNEL] Llego Instruccion F_CLOSE - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, nombre_archivo);

                //
                break;
            case F_SEEK:
                log_info(logger, "[KERNEL] Llego Instruccion F_SEEK");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int posicion = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case F_READ:
                log_info(logger, "[KERNEL] Llego Instruccion F_READ");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_WRITE:
                log_info(logger, "[KERNEL] Llego Instruccion F_WRITE");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_TRUNCATE:
                log_info(logger, "[KERNEL] Llego Instruccion F_TRUNCATE");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int tamanio_archivo = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case WAIT:
                //
                log_info(logger, "[KERNEL] Llego Instruccion WAIT");
                recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                manejar_wait(proceso, recurso);
                //
                break;
            case SIGNAL:
                //
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL");
                recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                manejar_signal(proceso, recurso);
                //
                break;
            case CREATE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion CREATE_SEGMENT");
                id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                tamanio_segmento = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                //
                break;
            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");
                //
                manejar_yield(proceso);
                //
                break;
            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");
                //
                manejar_exit(proceso);
                //
                break;
            default:
                log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
                break;
            }


               log_info(logger, "[KERNEL] Llego PCB %d de CPU", pcb->PID);
            break;

        case OP_PCB:
            log_info(logger, "[KERNEL] Llego PCB");
            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            PCB *pcb = obtener_paquete_pcb(socket_cpu);
            log_info(logger, "[KERNEL] Llego PCB %d de CPU", pcb->PID);

            reemplazar_pcb_en_procesos(pcb);

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

            Lista *lista_parametros = obtener_paquete_como_lista(socket_cpu);
            char* c = (char *)list_get(lista_parametros, 0);

            CODIGO_INSTRUCCION codigo_instruccion = *(int32_t *)list_get(lista_parametros, 0);

            char *nombre_archivo, recurso;
            int direccion_fisica, cant_bytes;
            int id_segmento, tamanio_segmento;

            PAQUETE *paquete_instruccion = crear_paquete(INSTRUCCION);

            switch (codigo_instruccion)
            {
            case IO:
                //

                int tiempo_io = *(int32_t *)list_get(lista_parametros, 1);
                log_info(logger, "[KERNEL] Llego Instruccion IO - Proceso PID:<%d> - Tiempo IO : <%d>", proceso->pcb->PID, tiempo_io);
                //
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%d>", proceso->pcb->PID, proceso->estado);
                proceso->estado = BLOCK;
                log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%d>", proceso->pcb->PID, proceso->estado);

                Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
                proceso_io->PID = pcb->PID;
                proceso_io->tiempo_bloqueado = tiempo_io;

                queue_push(cola_io, proceso_io);

                sem_post(&semaforo_io);
                //
                break;
            case F_OPEN:
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                log_info(logger, "[KERNEL] Llego Instruccion F_OPEN - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, nombre_archivo);

                // CODIGO_INSTRUCCION* f_open = F_OPEN;
                // agregar_a_paquete(paquete_instruccion,&f_open,sizeof(CODIGO_INSTRUCCION));
                // enviar_paquete_a_servidor(paquete, socket_file_system);
                // char *mensaje = obtener_mensaje_del_servidor(socket_file_system);
                // log_info(logger, "KERNEL: Recibi un mensaje de FS con motivo de F_OPEN:%s", mensaje);
                //
                break;
            case F_CLOSE:
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                log_info(logger, "[KERNEL] Llego Instruccion F_CLOSE - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, nombre_archivo);

                //
                break;
            case F_SEEK:
                log_info(logger, "[KERNEL] Llego Instruccion F_SEEK");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int posicion = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case F_READ:
                log_info(logger, "[KERNEL] Llego Instruccion F_READ");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_WRITE:
                log_info(logger, "[KERNEL] Llego Instruccion F_WRITE");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                direccion_fisica = *(int32_t *)list_get(lista_parametros, 2);
                cant_bytes = *(int32_t *)list_get(lista_parametros, 3);
                //
                break;
            case F_TRUNCATE:
                log_info(logger, "[KERNEL] Llego Instruccion F_TRUNCATE");
                nombre_archivo = string_duplicate((char *)list_get(lista_parametros, 1));
                int tamanio_archivo = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case WAIT:
                //
                log_info(logger, "[KERNEL] Llego Instruccion WAIT");
                recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                manejar_wait(proceso, recurso);
                //
                break;
            case SIGNAL:
                //
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL");
                recurso = string_duplicate((char *)list_get(lista_parametros, 1));
                manejar_signal(proceso, recurso);
                //
                break;
            case CREATE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion CREATE_SEGMENT");
                id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                tamanio_segmento = *(int32_t *)list_get(lista_parametros, 2);
                //
                break;
            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                id_segmento = *(int32_t *)list_get(lista_parametros, 1);
                //
                break;
            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");
                //
                manejar_yield(proceso);
                //
                break;
            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");
                //
                manejar_exit(proceso);
                //
                break;
            default:
                log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
                break;
            }


        }
    }
}
*/

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

        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s>", proceso->pcb->PID, proceso->estado);
        proceso->estado = READY;
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%s>", proceso->pcb->PID, proceso->estado);

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
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s>", proceso->pcb->PID, proceso->estado);
        proceso->estado = EXIT;
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%s>", proceso->pcb->PID, proceso->estado);
        return;
    }

    if (recurso->instancias > 0)
    {
        recurso->instancias -= 1;
    }
    else
    {
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s>", proceso->pcb->PID, proceso->estado);
        proceso->estado = BLOCK;
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%s>", proceso->pcb->PID, proceso->estado);

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
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s>", proceso->pcb->PID, proceso->estado);
        proceso->estado = EXIT;
        log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%s>", proceso->pcb->PID, proceso->estado);
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

Instruccion *buscar_instruccion_por_counter(PCB *pcb)
{

    Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

    Instruccion *instruccion = list_get(proceso->pcb->instrucciones, pcb->program_counter);

    return instruccion;
}

void manejar_yield(Proceso *proceso)
{
    pthread_mutex_lock(&mx_procesos);
    log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s>", proceso->pcb->PID, proceso->estado);
    proceso->estado = READY;
    log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%s>", proceso->pcb->PID, proceso->estado);
    queue_push(procesos, proceso);
    pthread_mutex_unlock(&mx_procesos);
}

void manejar_exit(Proceso *proceso)
{
    log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s>", proceso->pcb->PID, proceso->estado);
    proceso->estado = EXIT;
    log_info(logger, "[KERNEL] Proceso PID:<%d> - Estado Actual: <%s>", proceso->pcb->PID, proceso->estado);
    // liberar recursos
}

int obtener_Codigo_Instruccion_numero(char* instruccion) {
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

    // Valor por defecto en caso de que la instrucción no coincida con ninguna del enum
    return EXIT; // Podrías elegir cualquier valor de CODIGO_INSTRUCCION aquí
}