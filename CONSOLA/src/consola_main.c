#include "consola_utils.h"

int main()
{
    t_log *logger = log_create("./cfg/proceso1.log", "PROCESO1", true, LOG_LEVEL_INFO);
    log_info(logger, "Soy el proceso 1! %s", mi_funcion_compartida());

    inicializar_logger_consola();
    inicializar_config_consola();

    if(conectar_con_kernel() == SUCCESS)
    {
        enviar_instrucciones_a_kernel();
        escuchar_kernel();
        desconectar_con_kernel();
    }

    terminar_consola();

    return 0;
}