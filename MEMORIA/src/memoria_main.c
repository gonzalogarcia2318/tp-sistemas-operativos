#include "memoria_utils.h"

int main()
{
    iniciar_logger_memoria();

    iniciar_config_memoria();

    if(iniciar_servidor_memoria() == SUCCESS)
    {
        conectar_con_file_system();

        conectar_con_cpu();
        
        conectar_con_kernel();

        pthread_join(hilo_file_system, NULL);
        pthread_join(hilo_cpu, NULL);
        pthread_join(hilo_kernel, NULL);
    }

    crear_estructuras_administrativas();
    
    //
    
    terminar_ejecucion();

    return EXIT_SUCCESS;
}
