#include <consola_utils.h>

int main()
{
    inicializar_logger_consola();
    inicializar_config_consola();

    if(conectar_con_kernel() == SUCCESS)
    {
        enviar_instrucciones_a_kernel();
        escuchar_kernel();
        desconectar_con_kernel();
    }

    terminar_consola();

    return SUCCESS;
}