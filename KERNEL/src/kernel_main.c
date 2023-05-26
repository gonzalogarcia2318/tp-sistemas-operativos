#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]
// cola_ready: [1, 2, 3]
// cola_bloqueados: [5]


t_queue *cola_ready;

sem_t semaforo_planificador;

sem_t semaforo_ejecutando;


int main()
{

    // TODO: Destruir la lista al final.
    procesos = list_create();

    sem_init(&semaforo_new, 0, 0);

    iniciar_logger_kernel();

    iniciar_config_kernel();

    if(iniciar_servidor_kernel() == SUCCESS)
    {
        conectar_con_memoria();

        conectar_con_file_system();

        conectar_con_cpu();

        conectar_con_consola(); 

    }


    // trabarse hasta que exista un pcb para planificar
    // nos destrabamos cuando kernel_thead.c avisa que se creo un pcb
    sem_wait(&semaforo_new);

    Hilo hilo_planificador;
    pthread_create(&hilo_planificador, NULL, (void *)planificar, NULL);
    pthread_join(hilo_planificador);

     //manejar_proceso_consola();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}

void planificar(){

    // grado multiprogramacion
    sem_init(&semaforo_planificador, 0, 1);

    sem_init(&semaforo_ejecutando, 0, 1);

    while(true){
    
        sem_wait(&semaforo_planificador);

        bool en_new(Proceso proceso){
            return proceso.estado == NEW;
        }

        t_list* procesos_en_new = list_filter((void*) en_new);

        if(list_size(procesos_en_new) > 0){
            Proceso proceso_para_ready = list_get(procesos_en_new, 0);
            proceso_para_ready.estado = READY;
            queue_push(cola_ready, proceso_para_ready);
            log_info("Poner en ready proceso %d", proceso_para_ready.pcb.PID);
        }

        sem_wait(&semaforo_ejecutando);

        if(!queue_is_empty(cola_ready)){
            Proceso proceso_a_ejecutar = queue_pop(cola_ready);
            proceso_a_ejecutar.estado = EXEC;
            // destrabar ready
            log_info("Ejecutar proceso %d", proceso_a_ejecutar.pcb.PID);
            sleep(5);
            // destrabar semaforo ejecutar
        } else {
            // hay que chequear en que estado esta el resto
        }


        

    /*

        // semaforo: semaforo_ready va a tener tantas instancias como grado_multiprogramacion
        wait semaforo semaforo_ready {
            
            agarramos el 1ero de fifo de procesos que esten en NEW
            marcamos como READY

            wait semaforo semaforo_execute {
                agarrar el 1ero de fifo de procesos que esten en READY
                mandamos estructuras a memoria principal

                lo mandamos a ejecutar
                marcamos como EXEC
                cuando se termina de ejecutar -> destrabamos -> signal semaforo semaforo_ready 


                cuando se termina de ejecutar -> destrabamos -> signal semaforo semaforo_execute
                
            }
            

        }
    */
    }
}
