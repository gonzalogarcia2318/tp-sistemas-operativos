#include "file_system_utils.h"

int main()
{
    iniciar_logger_file_system();

    iniciar_config_file_system();

    if(iniciar_servidor_file_system() == SUCCESS)
    {
        if(conectar_con_memoria() == SUCCESS)
            conectar_con_kernel();
        else
            return EXIT_FAILURE;
    }

    iniciar_config_superbloque();

    if(levantar_bitmap(FileSystemConfig.PATH_BITMAP) == FAILURE)
        return EXIT_FAILURE;
    

    //...

    pthread_join(hilo_fileSystem, NULL); //LO PASO PARA ACA PARA QUE NO FINALICE EL MÓDULO SI DETACH NI SE BLOQUEE ANTES DE INICIALIZAR LAS ESTRUCTURAS NECESARIAS
    
    terminar_ejecucion();

    return EXIT_SUCCESS;
}


