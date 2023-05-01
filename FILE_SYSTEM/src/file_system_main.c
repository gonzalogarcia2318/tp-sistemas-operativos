#include "file_system_utils.h"

int main()
{
    iniciar_logger_file_system();

    iniciar_config_file_system();

    iniciar_servidor_file_system(); //COMPROBAR QUE SE HAYA CONECTADO (SUCCESS)

    //conectar_con_memoria();

    conectar_con_kernel();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


