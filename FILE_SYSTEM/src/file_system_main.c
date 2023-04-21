#include <file_system_utils.h>

int main()
{
    iniciar_logger_fileSystem();

    iniciar_config_fileSystem();

    iniciar_servidor_fileSystem();

    //conectar_con_memoria();

    conectar_con_kernel();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


