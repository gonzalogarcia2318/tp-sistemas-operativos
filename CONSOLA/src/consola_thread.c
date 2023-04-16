#include <consola_thread.h>

void enviar_instrucciones_a_kernel()
{
    //COMPLETAR
}

void escuchar_kernel()
{
    while(1)
    {
        switch (obtener_codigo_operacion(socket_kernel))
        {
        case DESCONEXION:
            log_warning(logger,"[CONSOLA]: Kernel se desconecto.");
            return;
        
        default:
            continue;
        }

        break;
    }
}
