#include "consola_utils.h"

int main()
{

    // TODO_A: RECIBIR PATHS DE CONFIG Y PSEUDOCODIGO POR ARGUMENTOS

    char *path_instrucciones = "./pseudocodigo2.txt";

    inicializar_logger_consola();
    inicializar_config_consola();

    t_list *instrucciones = leer_instrucciones(path_instrucciones);

    log_info(logger, "IP_KERNEL: %s", config_get_string_value(config, "IP_KERNEL"));

    if (conectar_con_kernel() == SUCCESS)
    {
        // GONZI <<<<
        //  -------------------------
        // CHILY >>>>

        // ENVIAR UN PAQUETE CON LAS INSTRUCCIONES A KERNEL
        enviar_instrucciones_a_kernel();

        // RECIBIR QUE LLEGARON LAS INSTRUCCIONES BIEN A KERNEL (bloqueante)

        // QUEDARSE ESPERANDO A QUE KERNEL ENVIE UN MENSAJE TERMINANDO LA EJECUCION PARA SALIR (bloqueante)
        // escuchar_kernel();

        desconectar_con_kernel();
    }

    terminar_consola();

    return 0;
}