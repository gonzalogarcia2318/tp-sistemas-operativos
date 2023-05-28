#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]
// cola_ready: [1, 2, 3]
// cola_bloqueados: [5]

t_queue *cola_ready;

sem_t semaforo_planificador;

sem_t semaforo_ejecutando;

sem_t semaforo_new;

pthread_mutex_t mx_procesos;

t_list *procesos;

int main()
{

    // TODO: Destruir la lista al final.
    procesos = list_create();
    cola_ready = queue_create();

    sem_init(&semaforo_new, 0, 0);
    pthread_mutex_init(&mx_procesos, NULL);


    iniciar_logger_kernel();

    iniciar_config_kernel();

    if (iniciar_servidor_kernel() == SUCCESS)
    {
        conectar_con_memoria();

        conectar_con_file_system();

        conectar_con_cpu();

        conectar_con_consola();
    }

    /*
        Hilo hilo_planificador;
        pthread_create(&hilo_planificador, NULL, (void *)planificar, NULL);
        pthread_join(hilo_planificador,NULL);
    */
    // manejar_proceso_consola();

    // grado multiprogramacion
    sem_init(&semaforo_planificador, 0, 1);

    sem_init(&semaforo_ejecutando, 0, 1);

    sem_wait(&semaforo_new); // Para que no empiece sin que no haya ningun proceso

    while (true)
    {
        log_info(logger, "Semaforo ready");
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

        log_info(logger, "Semaforo ejecutando");
        sem_wait(&semaforo_ejecutando); // Ejecuta uno a la vez

        log_info(logger, "Size cola ready: %d", queue_size(cola_ready));

        if (!queue_is_empty(cola_ready))
        {
            Proceso *proceso_a_ejecutar = (Proceso *)queue_pop(cola_ready);
            proceso_a_ejecutar->estado = EXEC;
            
            // destrabar ready
            sem_post(&semaforo_planificador);
            log_info(logger, "Proceso %d -> EXEC", (proceso_a_ejecutar->pcb)->PID);
            log_info(logger, "EJECUTANDO proceso %d", (proceso_a_ejecutar->pcb)->PID);
            sleep(5);
            // Enviar PCB a CPU

            sem_post(&semaforo_ejecutando);
            // destrabar semaforo ejecutar
        }
        else
        {
            sem_post(&semaforo_ejecutando);
        }
    }

    terminar_ejecucion();

    return EXIT_SUCCESS;
}
