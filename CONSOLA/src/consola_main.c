#include "consola_utils.h"

int main()
{

    // TODO_A: RECIBIR PATHS DE CONFIG Y PSEUDOCODIGO POR ARGUMENTOS

    inicializar_logger_consola();
    inicializar_config_consola();

    log_info(logger, "IP_KERNEL: %s", config_get_string_value(config, "IP_KERNEL"));

    if(conectar_con_kernel() == SUCCESS)
    {
        // TODO_A
        // LEER INSTRUCCIONES DEL ARCHIVO

        // PARSEAR INSTRUCCIONES
        // vamos a parsear cada instruccion a un struct propio nuestro
        // SET A 2
        // por ejemplo: {instruccion: SET, parametro1: A, parametro2: 2]}


        // GONZI <<<<
        //  -------------------------
        // CHILY >>>>
        
        // ENVIAR UN PAQUETE CON LAS INSTRUCCIONES A KERNEL
        enviar_instrucciones_a_kernel();

        // RECIBIR QUE LLEGARON LAS INSTRUCCIONES BIEN A KERNEL (bloqueante)

        // QUEDARSE ESPERANDO A QUE KERNEL ENVIE UN MENSAJE TERMINANDO LA EJECUCION PARA SALIR (bloqueante)
        //escuchar_kernel();


        
        desconectar_con_kernel();
    }

    terminar_consola();

    return 0;
}