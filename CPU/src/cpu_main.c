#include <cpu_utils.h>

int main()
{
    iniciar_logger_cpu();

    iniciar_config_cpu();

    iniciar_servidor_cpu();

    conectar_con_kernel();

    //conectar_con_memoria();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


