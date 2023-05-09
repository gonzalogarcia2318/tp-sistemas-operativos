#include "cpu_utils.h"

//bool esExit = false;//posiblemente combinar ambos bool.
//bool esYield = false;

void iniciar_logger_cpu()
{
    logger = log_create(ARCHIVO_LOGGER, "CPU", 1, LOG_LEVEL_INFO);
    log_info(logger, "[CPU]: Logger creado correctamente");
}

void iniciar_config_cpu()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_cpu(config);
    log_info(logger,"[CPU]: Archivo Config creado y rellenado correctamente");
}

int iniciar_servidor_cpu()
{
  log_info(logger, "[CPU]: Iniciando Servidor ...");
  socket_cpu = iniciar_servidor(CPUConfig.IP, CPUConfig.PUERTO_ESCUCHA);

  if (socket_kernel < 0)
  {
    log_error(logger, "[CPU]: Error intentando iniciar Servidor.");
    return FAILURE;
  }

  log_info(logger, "[CPU]: Servidor iniciado correctamente.");
  return SUCCESS;
}

// Conectar con cliente de cpu? - esperar kernel
void conectar_con_kernel()
{
    log_info(logger, "[MEMORIA]: Esperando conexiones de KERNEL..");
    socket_kernel = esperar_cliente(socket_cpu);
    log_info(logger, "[MEMORIA]: Conexión de KERNEL establecida.");

    pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete_kernel, (void *)socket_kernel);
    pthread_join(hilo_kernel, NULL);
}

int conectar_con_memoria()
{
	 log_info(logger, "[CPU] conectando con memoria...");
      log_info(logger, CPUConfig.PUERTO_MEMORIA);
	    socket_memoria = crear_conexion_con_servidor(CPUConfig.IP_MEMORIA, CPUConfig.PUERTO_MEMORIA);

	    if(socket_memoria < 0)
	    {
	        log_info(logger,"[CPU]: Error al conectar con memoria. Finalizando Ejecucion");
	        log_error(logger,"[CPU]: memoria no está disponible");

	        return FAILURE;

	    }
	    log_info(logger, "[CPU]: Conexion con Memoria: OK %d", socket_memoria);

	    return SUCCESS;
}

void terminar_ejecucion(){
    log_warning(logger, "[CPU]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}

/*
PCB* o void recibir_instrucciones(PCB){ 
    Instrucciones* = List_get(PCB.Instrucciones); 
    Instruccion* Prox_instruccion; 
    
    while(!esExit || !esYield)
    {
    
        Prox_Instruccion = List_get(PCB.Instrucciones,IP); //FORMA PARTE DEL FETCH
        
        decode_instruccion(Prox_Instruccion);
        
        ejecutar_instruccion(Prox_Instruccion);

        ...

        PCB.IP ++;
    }
    
    return PCB o sin return...
}

void decode_instruccion("Struct_Instruccion"){

    IF(ES_SET)
    {
        aplicar_retardo(CONFIG.RETARDO);
    }
}

*/
/*
void ejecutar_instruccion("Struct_Instruccion"){ //EXECUTE
    
        switch ("Struct.NombreInstruccion")
        {
            case "SET": 
                asignar_a_registro(Valor,Registro);
                break;
            
            case "YIELD":
                breack;

            case "EXIT":
                esExit = true:
                break;
            
            default:
                log_error(logger,"[CPU]: Codigo de Instruccion no encontrado");
                break;
        }
    }
    
}
*/