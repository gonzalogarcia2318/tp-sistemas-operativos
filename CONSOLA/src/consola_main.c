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

    log_info(logger, "[CONSOLA]: pseudocodigo_path: %s.", pseudocodigo_path);
    log_info(logger, "[CONSOLA]: config_path: %s.", config_path);



    if (conectar_con_kernel() == SUCCESS)
    {
        enviar_instrucciones_a_kernel(instrucciones);
        
        escuchar_kernel();

        desconectar_con_kernel();
    }

    list_destroy_and_destroy_elements(instrucciones, liberar_instruccion);

    terminar_consola();

    return 0;
}

void liberar_instruccion(Instruccion *instruccion)
{
    if (strcmp(instruccion->nombreInstruccion, "SET") == 0 
        || strcmp(instruccion->nombreInstruccion, "MOV_IN") == 0
        || strcmp(instruccion->nombreInstruccion, "MOV_OUT") == 0)
    {
        free(instruccion->registro);
    }

    if (strcmp(instruccion->nombreInstruccion, "SET") == 0)
    {
        free(instruccion->valor);
    }

    if (strcmp(instruccion->nombreInstruccion, "F_OPEN") == 0 || strcmp(instruccion->nombreInstruccion, "F_CLOSE") == 0
        || strcmp(instruccion->nombreInstruccion, "F_SEEK") == 0
        || strcmp(instruccion->nombreInstruccion, "F_READ") == 0 || strcmp(instruccion->nombreInstruccion, "F_WRITE") == 0
        || strcmp(instruccion->nombreInstruccion, "F_TRUNCATE") == 0)
    {
        free(instruccion->nombreArchivo);
    }

    if (strcmp(instruccion->nombreInstruccion, "WAIT") == 0 || strcmp(instruccion->nombreInstruccion, "SIGNAL") == 0)
    {
        free(instruccion->recurso);
    }
    
    free(instruccion->nombreInstruccion);
    free(instruccion);
}