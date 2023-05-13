#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]
// cola_ready: [1, 2, 3]
// cola_bloqueados: [5]

int main()
{
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

    // marcamos PCB recien creada como NEW 

    // EMPEZAR PLANIFICACION
    /*
    while (haya procesos en new, ready, exec, bloqueado){

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
    // manejar_proceso_consola();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


