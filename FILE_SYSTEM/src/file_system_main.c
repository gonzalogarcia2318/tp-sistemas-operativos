#include "file_system_utils.h"

int main(int argc, char** argv)
{
    if(iniciar_logger_file_system() == FAILURE)
        return EXIT_FAILURE;

    char* config_path = argv[1];

    if(iniciar_config_file_system(config_path) == FAILURE)
        return EXIT_FAILURE;

    if(iniciar_servidor_file_system() == SUCCESS)
    {
        if(conectar_con_memoria() == SUCCESS)
        {
            //conectar_con_kernel();

            if(iniciar_config_superbloque() == FAILURE)
                return EXIT_FAILURE;

            if(levantar_bitmap(FileSystemConfig.PATH_BITMAP) == FAILURE)
                return EXIT_FAILURE;
        
            if(iniciar_archivo_de_bloques(FileSystemConfig.PATH_BLOQUES) == FAILURE)
                return EXIT_FAILURE;

                // crear_archivo("Test");
                // ejecutar_f_truncate("Test",256);
                // ejecutar_f_truncate("Test",62);

                // crear_archivo("Prueba");
                // ejecutar_f_truncate("Prueba",64);
                // ejecutar_f_truncate("Prueba",192);

            conectar_con_kernel();
          
        }
        else
            return EXIT_FAILURE;
    }

    // pthread_join(hilo_fileSystem, NULL); //LO PASO PARA ACA PARA QUE NO FINALICE EL MÓDULO SI DETACH NI SE BLOQUEE ANTES DE INICIALIZAR LAS ESTRUCTURAS NECESARIAS
    
    terminar_ejecucion();

    return EXIT_SUCCESS;
}


