#include <cpu_utils.h>

int main()
{
    iniciar_logger_cpu();

    iniciar_config_cpu();

    iniciar_servidor_cpu();

    conectar_con_kernel();

    int conexionMemoria = conectar_con_memoria();

    if( conexionMemoria== SUCCESS)
    {
    	enviar_mensaje_a_servidor(mensaje,  socketCliente) //reemplazar parametros
        liberar_conexion_con_servidor(conexionMemoria);
    }

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


