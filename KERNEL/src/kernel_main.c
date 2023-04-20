#include <kernel_utils.h>

int main()
{
    iniciar_logger_kernel();

    iniciar_config_kernel();

    iniciar_servidor_kernel();

    //conectar_con_memoria();

    //conectar_con_filesystem();

    conectar_con_cpu();

    conectar_con_consola();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


