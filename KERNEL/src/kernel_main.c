#include "kernel_utils.h"

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

    manejar_proceso_consola();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


