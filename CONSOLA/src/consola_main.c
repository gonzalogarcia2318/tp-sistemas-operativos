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
    //pseudocodigo_path = "deadlock1.txt";
    
    inicializar_config_consola(config_path);

    t_list *instrucciones = leer_instrucciones(pseudocodigo_path);

    log_info(logger, "[CONSOLA]: %s.", pseudocodigo_path);


    if (conectar_con_kernel() == SUCCESS)
    {
        enviar_instrucciones_a_kernel(instrucciones);
        
        escuchar_kernel();

        desconectar_con_kernel();
    }

    terminar_consola();

    return 0;
}