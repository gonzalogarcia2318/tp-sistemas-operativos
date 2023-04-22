#include <memoria_utils.h>

int main()
{
    iniciar_logger_memoria();

    iniciar_config_memoria();

    iniciar_servidor_memoria();

    conectar_con_kernel();

    conectar_con_cpu();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}
