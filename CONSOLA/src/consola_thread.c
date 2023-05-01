#include "consola_thread.h"
#include "protocolo.h"

void enviar_instrucciones_a_kernel()
{
    log_info(logger,"[CONSOLA]:Enviando mensaje a KERNEL...");
    enviar_mensaje_a_servidor(ConsolaConfig.IP_KERNEL,socket_kernel);
    log_info(logger,"[CONSOLA]:Mensaje enviado.");

}

void escuchar_kernel()
{
    /*
    while (1)
    {
        switch (obtener_codigo_operacion(socket_kernel))
        {
        case DESCONEXION:
            log_warning(logger, "[CONSOLA]: Kernel se desconecto.");
            return;

        default:
            continue;
        }

        break;
    }
    */
}
