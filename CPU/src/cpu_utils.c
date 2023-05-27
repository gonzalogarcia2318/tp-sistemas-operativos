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

    int seguir = 1; //SI DEBE DEVOLVER CONTEXTO DE EJECUCIÓN => CAMBIAR A 0. CADA INSTRUCCIÓN LO DECIDE.
    
    while(seguir)
    {
        prox_instruccion = list_get(instrucciones,pcb->program_counter); //FORMA PARTE DEL FETCH
        
        decode_instruccion(prox_instruccion);
        
        ejecutar_instruccion(prox_instruccion, pcb);

        pcb->program_counter ++;
    }
}

bool esExit(Instruccion* Instruccion){
    bool es = !strcmp(Instruccion->nombreInstruccion,"EXIT");
    return es;
}
bool esYield(Instruccion* Instruccion){ //MEPA QUE NO VA
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
    
    if(requiere_traduccion(Instruccion))
    {
        Instruccion->direccionFisica = realizar_traduccion(Instruccion->direccionLogica);
    }
    
}


int ejecutar_instruccion(Instruccion* Instruccion, PCB* pcb) //EXECUTE //CADA INSTRUCCIÓN DEBE TENER SU log_warning(PID: <PID> - Ejecutando: <INSTRUCCION> - <PARAMETROS>)
{ 
    char* nombre_instru = string_duplicate(Instruccion->nombreInstruccion);
    PAQUETE* paquete = crear_paquete(INSTRUCCION);

    if(!strcmp(nombre_instru,"SET"))
    {
        log_info(logger,"CPU: Leí la instrucción SET en ejecutar_instruccion");
        asignar_a_registro(Instruccion->valor,Instruccion->registro, pcb);
        return 1;
    }
    else if(!strcmp(nombre_instru,"MOV_IN"))
    {
        ejecutar_mov_in(paquete,Instruccion,pcb);
        return 1;
        //Comunicación con Memoria: Memoria -> Registro
    }
    else if(!strcmp(nombre_instru,"MOV_OUT"))
    {
        ejecutar_mov_out(paquete,Instruccion,pcb);
        return 1;
        //Comunicación con Memoria: Registro -> Memoria
    }
    else if(!strcmp(nombre_instru,"I/O")) //TOM
    {
        ejecutar_IO(paquete,Instruccion,pcb);
        return 0;
        //Comunicación con Kernel: Unidades de tiempo que se bloquea. 
		//Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"F_OPEN")) //TOM
    {
        //Comunicación con Kernel: nombre_archivo
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"F_CLOSE")) //TOM
    {
        //Comunicación con Kernel: nombre_archivo
		//Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"F_SEEK")) //DIEGO
    {
        //Comunicación con Kernel: nombre_archivo, posición
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"F_READ")) //DIEGO
    {
        //Comunicación con Kernel: nombre_archivo, dire_lógica, cant_bytes
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"F_WRITE"))//DIEGO
    {
        //Comunicación con Kernel: nombre_archivo, dire_lógica, cant_bytes
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"F_TRUNCATE ")) //DIEGO
    {
        //Comunicación con Kernel: nombre_archivo, tamaño
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"WAIT")) //CAMIL
    {
        //Avisarle a Kernel que se bloquee
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"SIGNAL")) //CAMIL
    {
        //Avisarle a Kernel que se libere
        //Devolver Contexto de Ejecución
    }
    else if(!strcmp(nombre_instru,"CREATE_SEGMENT")) //CAMIL
    {
        //solicita al kernel la creación del segmento con el Id y tamaño.
    }
    else if(!strcmp(nombre_instru,"DELETE_SEGMENT ")) //CAMIL
    {
        //solicita al kernel que se elimine el segmento cuyo Id se pasa por parámetro.
    }
    else if(!strcmp(nombre_instru,"YIELD"))
    {
        log_info(logger,"CPU: Leí la instrucción YIELD en ejecutar_instruccion");
    }
    else if(!strcmp(nombre_instru,"EXIT"))
    {
        log_info(logger,"CPU: Leí la instrucción EXIT en ejecutar_instruccion");
    }
    else
    {
        log_error(logger,"[CPU]: Codigo de Instruccion no encontrado");
    } 

    eliminar_paquete(paquete);
}
 
void asignar_a_registro (char* valor , char* registro_instr, PCB* pcb)
{
    Registro_CPU* reg_cpu = pcb->registros_cpu;

    if(!strcmp(registro_instr,"AX"))
    {
        reg_cpu->valor_AX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"BX"))
    {
        reg_cpu->valor_BX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"CX"))
    {
        reg_cpu->valor_CX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"DX"))
    {
        reg_cpu->valor_DX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"EAX"))
    {
        reg_cpu->valor_EAX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"EBX"))
    {
        reg_cpu->valor_EBX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"ECX"))
    {
        reg_cpu->valor_ECX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"EDX"))
    {
        reg_cpu->valor_EDX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"RAX"))
    {
        reg_cpu->valor_RAX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"RBX"))
    {
        reg_cpu->valor_RBX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"RCX"))
    {
        reg_cpu->valor_RCX = string_duplicate(valor);
    }
    else if(!strcmp(registro_instr,"RDX"))
    {
        reg_cpu->valor_RDX = string_duplicate(valor);
    }
    else
    {
        log_error(logger,"CPU: ERROR AL ASIGNAR REGISTRO, NOMBRE DESCONOCIDO");
    }
}
char* obtener_valor_registro(Registro_CPU* registros_pcb,char* registro_buscado)
{
    char* valor = string_itoa(0); //solo lo inicializo, se tiene q pisar

    if(!strcmp(registro_buscado,"AX"))
    {
        valor = string_duplicate(registros_pcb->valor_AX);
    }
    else if(!strcmp(registro_buscado,"BX"))
    {
        valor = string_duplicate(registros_pcb->valor_BX);
    }
    else if(!strcmp(registro_buscado,"CX"))
    {
        valor = string_duplicate(registros_pcb->valor_CX);
    }
    else if(!strcmp(registro_buscado,"DX"))
    {
        valor = string_duplicate(registros_pcb->valor_DX);
    }
    else if(!strcmp(registro_buscado,"EAX"))
    {
        valor = string_duplicate(registros_pcb->valor_EAX);
    }
    else if(!strcmp(registro_buscado,"EBX"))
    {
        valor = string_duplicate(registros_pcb->valor_EBX);
    }
    else if(!strcmp(registro_buscado,"ECX"))
    {
        valor = string_duplicate(registros_pcb->valor_ECX);
    }
    else if(!strcmp(registro_buscado,"EDX"))
    {
        valor = string_duplicate(registros_pcb->valor_EDX);
    }
    else if(!strcmp(registro_buscado,"RAX"))
    {
        valor = string_duplicate(registros_pcb->valor_RAX);
    }
    else if(!strcmp(registro_buscado,"RBX"))
    {
        valor = string_duplicate(registros_pcb->valor_RBX);
    }
    else if(!strcmp(registro_buscado,"RCX"))
    {
        valor = string_duplicate(registros_pcb->valor_RCX);
    }
    else if(!strcmp(registro_buscado,"RDX"))
    {
        valor = string_duplicate(registros_pcb->valor_RDX);
    }
    else
    {
        log_error(logger,"CPU: ERROR AL BUSCAR REGISTRO, NOMBRE DESCONOCIDO");
    }
    return valor;
}

void aplicar_retardo(int32_t retardo)
{
    sleep(retardo);
}

bool requiere_traduccion(Instruccion* instruccion)
{
    char* nombre_instru = string_duplicate(instruccion->nombreInstruccion);

    if(!strcmp(nombre_instru,"MOV_IN") ||
        !strcmp(nombre_instru,"MOV_OUT") ||
        !strcmp(nombre_instru,"F_READ") ||
        !strcmp(nombre_instru,"F_WRITE")) 
            return true;
    
    else return false;
}

int32_t realizar_traduccion(int32_t dir_logica)
{ 
    int num_segmento = floor(dir_logica / CPUConfig.TAM_MAX_SEGMENTO);
    int desplazamiento_segmento = (dir_logica % CPUConfig.TAM_MAX_SEGMENTO);

    int32_t direccion_fisica = num_segmento * CPUConfig.TAM_MAX_SEGMENTO + desplazamiento_segmento;

    return direccion_fisica;
}

bool comprobar_segmentation_fault(int32_t dir_logica, int32_t tam_leer_escribir)
{
    int desplazamiento_segmento = (dir_logica % CPUConfig.TAM_MAX_SEGMENTO);

    return desplazamiento_segmento + tam_leer_escribir > CPUConfig.TAM_MAX_SEGMENTO;
}

void ejecutar_mov_in(PAQUETE* paquete,Instruccion* instruccion,PCB* pcb)
{

    log_warning(logger,"CPU: PID: <%d> - Ejecutando: <MOV_IN> - <REGISTRO:%s , DIRECCIÓN LÓGICA: %d>",
                    pcb->PID,
                    instruccion->registro, 
                    instruccion->direccionLogica
                );
    agregar_a_paquete(paquete,MOV_IN,sizeof(int));
    agregar_a_paquete(paquete,instruccion->direccionFisica,sizeof(int32_t));
    enviar_paquete_a_servidor(paquete,socket_memoria);

    char* valor = string_duplicate(obtener_mensaje_del_servidor(socket_memoria));
    //... SE BLOQUEA HASTA QUE RESPONDA
    
    int num_segmento = floor(instruccion->direccionLogica / CPUConfig.TAM_MAX_SEGMENTO);

    log_warning(logger,"CPU: PID: <%d> - Acción: <LEER> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",
                    pcb->PID,
                    num_segmento,
                    instruccion->direccionFisica,
                    valor
                );
    asignar_a_registro(valor,instruccion->registro,pcb);

}

void ejecutar_mov_out(PAQUETE* paquete,Instruccion* instruccion,PCB* pcb)
{
    log_warning(logger,"CPU: PID: <%d> - Ejecutando: <MOV_OUT> - <DIRECCIÓN LÓGICA: %d, REGISTRO: %s>",
                    pcb->PID,
                    instruccion->direccionLogica,
                    instruccion->registro
                );
    char* registro = string_duplicate(instruccion->registro);
    char* valor_registro = string_duplicate(obtener_valor_registro(pcb->registros_cpu,registro));

    agregar_a_paquete(paquete,MOV_OUT,sizeof(int));
    agregar_a_paquete(paquete,instruccion->direccionFisica,sizeof(int32_t));
    agregar_a_paquete(paquete,valor_registro,sizeof(char*));
    enviar_paquete_a_servidor(paquete,socket_memoria);
    char* mensaje = obtener_mensaje_del_servidor(socket_memoria);
    //... SE BLOQUEA HASTA QUE RESPONDA
    log_info(logger,"CPU: Recibi un mensaje de MEMORIA como RTA a MOV_OUT: <%s>",mensaje);
    int num_segmento = floor(instruccion->direccionLogica / CPUConfig.TAM_MAX_SEGMENTO);

    log_warning(logger,"CPU: PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",
                    pcb->PID,
                    num_segmento,
                    instruccion->direccionFisica,
                    valor_registro);
}

void ejecutar_IO(PAQUETE* paquete,Instruccion* instruccion,PCB* pcb)
{
    log_warning(logger,"CPU: PID: <%d> - Ejecutando: <I/O> - <TIEMPO: %d>",
                    pcb->PID,
                    instruccion->tiempo
                );
    //MANDO DOS PAQUETES, CHECKEAR SI ES CORRECTO.
    enviar_pcb(pcb); 
    agregar_a_paquete(paquete,IO,sizeof(int));
    agregar_a_paquete(paquete,instruccion->tiempo,sizeof(int32_t));
    enviar_paquete_a_cliente(paquete,socket_kernel);
}

void ejecutar_f_open(PAQUETE* paquete,Instruccion* instruccion,PCB* pcb)
{
    log_warning(logger,"CPU: PID: <%d> - Ejecutando: <F_OPEN> - <NOMBRE_ARCHIVO: %s>",
                    pcb->PID,
                    instruccion->nombreArchivo
                );
    agregar_a_paquete(paquete,F_OPEN,sizeof(int));
    agregar_a_paquete(paquete,instruccion->nombreArchivo,sizeof(char*));
    enviar_paquete_a_cliente(paquete, socket_kernel);
}

void ejecutar_f_close(PAQUETE* paquete,Instruccion* instruccion,PCB* pcb)
{
    log_warning(logger,"CPU: PID: <%d> - Ejecutando: <F_CLOSE> - <NOMBRE_ARCHIVO: %s>",
                    pcb->PID,
                    instruccion->nombreArchivo
                );
    agregar_a_paquete(paquete,F_CLOSE,sizeof(int));
    agregar_a_paquete(paquete,instruccion->nombreArchivo,sizeof(char*));
    enviar_paquete_a_cliente(paquete, socket_kernel);
}