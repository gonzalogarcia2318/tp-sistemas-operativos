#include "cpu_utils.h"

Logger *logger;
Config *config;
Hilo hilo_kernel;
int socket_cpu;
int socket_memoria;
int socket_cliente;
int socket_kernel;

int main()
{
    iniciar_logger_cpu();

    iniciar_config_cpu();

    iniciar_servidor_cpu();

    conectar_con_kernel();

    int conexionMemoria = conectar_con_memoria();

    if( conexionMemoria== SUCCESS)
    {
        char *mensaje;
    	enviar_mensaje_a_servidor(mensaje,  socket_cliente); //reemplazar parametros
        liberar_conexion_con_servidor(conexionMemoria);
    }

    terminar_ejecucion();

    return EXIT_SUCCESS;
}


