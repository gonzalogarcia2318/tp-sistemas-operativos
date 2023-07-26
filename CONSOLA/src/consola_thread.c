#include "consola_thread.h"
#include "protocolo.h"

void enviar_instrucciones_a_kernel(t_list* instrucciones_archivo)
{

    log_info(logger, "[CONSOLA]: Creando paquete de instrucciones");
        
    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL: preparando paquete");

    PAQUETE *paquete_instrucciones = crear_paquete(INSTRUCCIONES);

    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL:: Serializando instrucciones");

    paquete_instrucciones->buffer = serializar_instrucciones(instrucciones_archivo);

    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL: enviar");

    enviar_paquete_a_servidor(paquete_instrucciones, socket_kernel);

    eliminar_paquete(paquete_instrucciones);

}

void escuchar_kernel()
{
    
    while (1)
    {
        switch (obtener_codigo_operacion(socket_kernel))
        {
        case DESCONEXION:
            log_warning(logger, "[CONSOLA]: Kernel se desconecto.");
            return;

        case RECEPCION_OK:
            log_info(logger, "[CONSOLA]: KERNEL CONFIRMO RECEPCION DE INSTRUCCIONES.");

            log_info(logger, "[CONSOLA]: Esperando finalizacion del proceso...");
            break;

        case PROCESO_FINALIZADO:
            log_info(logger, "[CONSOLA]: FINALIZO EL PROCESO.");
            return;
        default:
            continue;
        }

    }
    
}
