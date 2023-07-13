#include "memoria_utils.h"

int main(int argc, char** argv)
{
    iniciar_logger_memoria();

    char* config_path = argv[1];

    if(iniciar_config_memoria(config_path) == FAILURE)
        return EXIT_FAILURE;

    if(iniciar_servidor_memoria() == SUCCESS)
    {
        conectar_con_file_system();

        conectar_con_cpu();
        
        conectar_con_kernel();

        crear_estructuras_administrativas();
    }

    pthread_join(hilo_file_system, NULL);
    pthread_join(hilo_cpu, NULL);
    pthread_join(hilo_kernel, NULL);
    
    terminar_ejecucion_memoria();

    return EXIT_SUCCESS;
}
