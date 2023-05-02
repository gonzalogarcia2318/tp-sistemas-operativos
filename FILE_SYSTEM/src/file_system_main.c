#include "file_system_utils.h"

int main()
{
    iniciar_logger_file_system();

    iniciar_config_file_system();

    if(iniciar_servidor_file_system() == SUCCESS)
    {
        conectar_con_memoria();
        
        conectar_con_kernel();
    }
    terminar_ejecucion();

    return EXIT_SUCCESS;
}


