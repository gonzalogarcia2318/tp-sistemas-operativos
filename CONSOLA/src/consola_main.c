#include "consola_utils.h"

int main(int argc, char** argv)
{

    inicializar_logger_consola();
    
    if(argc != 3){
        log_error(logger, "[CONSOLA]: Error. Se requiere el path de configuracion y pseudocodigo.");
        return EXIT_FAILURE;
    }

    char *config_path = argv[1];
    char *pseudocodigo_path = argv[2];

    // SETEAR PATH ACA PARA PROBAR MAS FACIL. SACAR.
    pseudocodigo_path = "pseudocodigo.txt";
    
    inicializar_config_consola(config_path);

    t_list *instrucciones = leer_instrucciones(pseudocodigo_path);

    if (conectar_con_kernel() == SUCCESS)
    {
        //  -------------------------
        // CHILY >>>>
        sleep(5);
        // ENVIAR UN PAQUETE CON LAS INSTRUCCIONES A KERNEL
        enviar_instrucciones_a_kernel();

        // RECIBIR QUE LLEGARON LAS INSTRUCCIONES BIEN A KERNEL (bloqueante)
        //escuchar_kernel();

        // QUEDARSE ESPERANDO A QUE KERNEL ENVIE UN MENSAJE TERMINANDO LA EJECUCION PARA SALIR (bloqueante)

         escuchar_kernel();

        desconectar_con_kernel();
    }

    terminar_consola();

    return 0;
}