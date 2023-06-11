#include "cpu_utils.h"

Logger *logger;
Config *config;
Hilo hilo_kernel;
int socket_cpu;
int socket_memoria;
int socket_cliente;
int socket_kernel;

int main(int argc, char** argv)
{
    iniciar_logger_cpu();

    char* config_path = argv[1];

    if(iniciar_config_cpu(config_path) == FAILURE) 
        return EXIT_FAILURE;

    iniciar_servidor_cpu();

   
    int respuesta_conexion = conectar_con_memoria();

    if(respuesta_conexion != SUCCESS)
    {       
        terminar_ejecucion();
        return EXIT_FAILURE;
        // liberar_conexion_con_servidor(conexionMemoria);
        //free(mensaje); //Luego de inicializar el char* corresponde usar free para liberar la memoria utilizada
    }
    enviar_mensaje_a_servidor("HOLA! SOY CPU (¬‿¬)", socket_memoria); 
    log_info(logger, "Se envio el mensaje");
    conectar_con_kernel();

    terminar_ejecucion();

    return EXIT_SUCCESS;
}

