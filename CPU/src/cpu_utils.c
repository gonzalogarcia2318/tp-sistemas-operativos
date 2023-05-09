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


void recibir_instrucciones(PCB* pcb)
{ 
    t_list* instrucciones = list_duplicate(pcb->instrucciones);

    Instruccion* prox_instruccion;
    
    while(!esExit(prox_instruccion) && !esYield(prox_instruccion))
    {
        prox_instruccion = list_get(instrucciones,pcb->program_counter); //FORMA PARTE DEL FETCH
        
        decode_instruccion(prox_instruccion);
        
        ejecutar_instruccion(prox_instruccion, pcb);

        //...

        pcb->program_counter ++;
    }
}

bool esExit(Instruccion* Instruccion){
    bool es = !strcmp(Instruccion->nombreInstruccion,"EXIT");
    return es;
}
bool esYield(Instruccion* Instruccion){
    bool es = !strcmp(Instruccion->nombreInstruccion,"YIELD");
    return es;
}
bool esSet(Instruccion* Instruccion){
    bool es = !strcmp(Instruccion->nombreInstruccion,"SET");
    return es;
}

void decode_instruccion(Instruccion* Instruccion){

    if(esSet(Instruccion))
    {
        aplicar_retardo(CPUConfig.RETARDO_INSTRUCCION);
    }
    /*
    if(requiereTraduccionDireccionLogica(Instruccion)){
        realizar_traduccion()
    }
    */
}


void ejecutar_instruccion(Instruccion* Instruccion, PCB* pcb){ //EXECUTE
    
        switch (*Instruccion->nombreInstruccion)
        {
            case 'SET': 
                asignar_a_registro(Instruccion->valor,Instruccion->registro, pcb); 
                break;
            
            case 'YIELD':
                //......
                break;

            case 'EXIT': //NO HACE NADA, LO ELIMINO?
                break;
            
            default:
                log_error(logger,"[CPU]: Codigo de Instruccion no encontrado");
                break;
        }
    }
/*  
asignar_a_registro (int32_t valor , char* nombreRegistro, PCB* pcb) {
    int i = 0;
    for(i; i < sizeof(pcb->registros_cpu;i++))
    {
        
    }
}
*/
