#include "cpu_utils.h"

// bool esExit = false;//posiblemente combinar ambos bool.
// bool esYield = false;

void iniciar_logger_cpu()
{
    logger = log_create(ARCHIVO_LOGGER, "CPU", 1, LOG_LEVEL_INFO);
    log_info(logger, "[CPU]: Logger creado correctamente");
}

int iniciar_config_cpu(char* path)
{
    config = config_create(path);
    if(config == NULL)
    {
        log_error(logger,"[CPU]: ERROR AL INICIAR CONFIG INICIAL");
        return FAILURE;
    }
    rellenar_configuracion_cpu(config);
    log_info(logger, "[CPU]: Archivo Config creado y rellenado correctamente");
    return SUCCESS;
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
    log_info(logger, "[CPU]: Esperando conexiones de KERNEL..");
    socket_kernel = esperar_cliente(socket_cpu);
    log_info(logger, "[CPU]: Conexión de KERNEL establecida.");

    pthread_create(&hilo_kernel, NULL, (void *)manejar_paquete_kernel, (void *)socket_kernel);
    pthread_join(hilo_kernel, NULL);
}

int conectar_con_memoria()
{
    log_info(logger, "[CPU] conectando con memoria...");
    socket_memoria = crear_conexion_con_servidor(CPUConfig.IP_MEMORIA, CPUConfig.PUERTO_MEMORIA);

    if (socket_memoria < 0)
    {
        log_error(logger, "[CPU]: MEMORIA NO ESTÁ DISPONIBLE, FINALIZANDO EJECUCIÓN");
        return FAILURE;
    }
    log_info(logger, "[CPU]: Conexion con Memoria: OK %d", socket_memoria);
    return SUCCESS;
}

void terminar_ejecucion()
{
    log_warning(logger, "[CPU]: Finalizando ejecucion...");
    log_destroy(logger);
    config_destroy(config);
}

void manejar_instrucciones(PCB *pcb)
{

    Instruccion *prox_instruccion;

    int seguir = 1; // SI DEBE DEVOLVER CONTEXTO DE EJECUCIÓN ó SEG FAULT => CAMBIAR A 0.

    while (seguir)
    {
        prox_instruccion = list_get(pcb->instrucciones, pcb->program_counter); // FORMA PARTE DEL FETCH

        if (!decode_instruccion(prox_instruccion, pcb)) // SI SEGMENTATION FAULT
        {
            avisar_seg_fault_kernel(pcb, prox_instruccion);
            seguir = 0;
        }
        if (seguir)
        {
            seguir = ejecutar_instruccion(prox_instruccion, pcb);

            pcb->program_counter++;  
        }
    }
}

int decode_instruccion(Instruccion *Instruccion, PCB *pcb)
{
    if (esSet(Instruccion))
    {
        aplicar_retardo(CPUConfig.RETARDO_INSTRUCCION);
        return 1;
    }

    int32_t dire_logica = Instruccion->direccionLogica;
    if (requiere_traduccion(Instruccion))
    {
        if (!comprobar_segmentation_fault(dire_logica, Instruccion, pcb->tabla_segmentos))
        {
            Instruccion->direccionFisica = realizar_traduccion(dire_logica, pcb->tabla_segmentos);
            return 1; // NO SEG FAULT
        }
        else
        {
            return 0; // SEG FAULT
        }
    }
    return 1;
}
bool esSet(Instruccion *Instruccion)
{
    bool es = !strcmp(Instruccion->nombreInstruccion, "SET");
    return es;
}
void aplicar_retardo(int32_t retardo)
{
    int segundos = retardo/1000;
    log_info(logger,"Retraso de <%d> segundos por ejecutar instrucción SET",segundos);
    sleep(segundos);
}
bool requiere_traduccion(Instruccion *instruccion)
{
    if (!strcmp(instruccion->nombreInstruccion, "MOV_IN") ||
        !strcmp(instruccion->nombreInstruccion, "MOV_OUT") ||
        !strcmp(instruccion->nombreInstruccion, "F_READ") ||
        !strcmp(instruccion->nombreInstruccion, "F_WRITE"))
        return true;

    else
        return false;
}
int32_t realizar_traduccion(int32_t dir_logica, t_list *tabla_segmentos)
{
    imprimir_tabla_segmentos(tabla_segmentos);

    int num_segmento = obtener_num_segmento(dir_logica);

    int desplazamiento_segmento = obtener_desplazamiento_segmento(dir_logica);

    SEGMENTO *segmento_aux;
    SEGMENTO *segmento;
    for(int i = 0; i < list_size(tabla_segmentos); i++){
        segmento_aux = list_get(tabla_segmentos, i);
        if(segmento_aux->id == num_segmento){
            segmento = segmento_aux;
            break;
        }
    }
    log_info(logger, "desplazamiento: %d", desplazamiento_segmento);

    int32_t direccion_fisica = (segmento->base) + desplazamiento_segmento;

    log_info(logger, "direccion fisica: %d", direccion_fisica);
    return direccion_fisica;
}
int obtener_num_segmento(int32_t direccion_logica)
{
    return floor(direccion_logica / CPUConfig.TAM_MAX_SEGMENTO);
}

int obtener_desplazamiento_segmento(int32_t direccion_logica)
{
    return direccion_logica % CPUConfig.TAM_MAX_SEGMENTO;
}

bool comprobar_segmentation_fault(int32_t dir_logica, Instruccion *Inst, t_list *tabla_segmentos) // Tengo que usar Free() ?
{
    log_info(logger, "Comprobar SEG_FAULT para instruccion: %s", Inst->nombreInstruccion);

    int32_t tam_a_usar;
    if (!strcmp(Inst->nombreInstruccion, "MOV_IN"))
        tam_a_usar = obtener_tamanio_registro(Inst->registro);

    if (!strcmp(Inst->nombreInstruccion, "MOV_OUT"))
        tam_a_usar = obtener_tamanio_registro(Inst->registro);

    if (!strcmp(Inst->nombreInstruccion, "F_READ"))
        tam_a_usar = Inst->cantBytes;

    if (!strcmp(Inst->nombreInstruccion, "F_WRITE"))
        tam_a_usar = Inst->cantBytes;

    int num_segmento = obtener_num_segmento(dir_logica);

    int desplazamiento_segmento = obtener_desplazamiento_segmento(dir_logica);

    SEGMENTO *segmento = (SEGMENTO *)list_get(tabla_segmentos, num_segmento);
    int maximo = segmento->limite;

    log_info(logger, "Comprobando: num_segmento %d - desplazamiento_segmento %d - tam_a_usar: %d ---- maximo %d",
                     num_segmento, 
                     desplazamiento_segmento,
                     tam_a_usar,
                     maximo
            );

    return desplazamiento_segmento + tam_a_usar > maximo;
}

void avisar_seg_fault_kernel(PCB *pcb, Instruccion *instruccion)
{
    PAQUETE *paquete = crear_paquete(SEG_FAULT);
    int num_segmento = obtener_num_segmento(instruccion->direccionLogica);
    int desplazamiento = obtener_desplazamiento_segmento(instruccion->direccionLogica);
    log_error(logger, "PID: <%d>, Error SEG_FAULT - Segmento: <NUMERO SEGMENTO: %d> - Offset: <%d> - Tamaño: <%d>",
              pcb->PID,
              num_segmento,
              desplazamiento,
              instruccion->cantBytes);
    agregar_a_paquete(paquete,&pcb->PID,sizeof(int32_t));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

int ejecutar_instruccion(Instruccion *Instruccion, PCB *pcb) // EXECUTE //CADA INSTRUCCIÓN DEBE TENER SU log_warning(PID: <PID> - Ejecutando: <INSTRUCCION> - <PARAMETROS>)
{

    PAQUETE *paquete = crear_paquete(INSTRUCCION);

    //quitar_salto_de_linea(nombre_instru);

    if (!strcmp(Instruccion->nombreInstruccion, "SET"))
    {
        ejecutar_set(paquete, Instruccion, pcb);
        return 1;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "MOV_IN"))
    {
        ejecutar_mov_in(paquete, Instruccion, pcb);
        return 1;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "MOV_OUT"))
    {
        ejecutar_mov_out(paquete, Instruccion, pcb);
        return 1;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "I/O"))
    {
        ejecutar_IO(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "F_OPEN"))
    {
        ejecutar_f_open(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "F_CLOSE"))
    {
        ejecutar_f_close(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "F_SEEK"))
    {
        ejecutar_f_seek(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "F_READ"))
    {
        ejecutar_f_read(paquete, Instruccion, pcb); // DIR LOGICA?
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "F_WRITE"))
    {
        ejecutar_f_write(paquete, Instruccion, pcb); // DIR LOGICA?
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "F_TRUNCATE"))
    {
        ejecutar_f_truncate(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "WAIT")) // CAMIL
    {
        ejecutar_wait(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "SIGNAL")) // CAMIL
    {
        ejecutar_signal(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "CREATE_SEGMENT")) // CAMIL
    {
        ejecutar_create_segment(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "DELETE_SEGMENT")) // CAMIL
    {
        ejecutar_delete_segment(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "YIELD"))
    {
        ejecutar_yield(paquete, pcb);
        return 0;
    }
    else if (!strcmp(Instruccion->nombreInstruccion, "EXIT"))
    {
        ejecutar_exit(paquete, pcb);
        return 0;
    }
    else
    {
        log_error(logger, "[CPU]: Codigo de Instruccion no encontrado");
        eliminar_paquete(paquete);
        return 0;
    }
}

void asignar_a_registro(char *valor, char *registro_instr, PCB *pcb)
{
    Registro_CPU *reg_cpu = &pcb->registros_cpu;
    //quitar_salto_de_linea(valor);

    if(!strcmp(registro_instr,"AX"))
    {
        strncpy(reg_cpu->valor_AX, valor, 4);
        reg_cpu->valor_AX[4] = '\0';
    }
    else if(!strcmp(registro_instr,"BX"))
    {
        strncpy(reg_cpu->valor_BX, valor, 4);
        reg_cpu->valor_BX[4] = '\0';
    }
    else if(!strcmp(registro_instr,"CX"))
    {
        strncpy(reg_cpu->valor_CX, valor, 4);
        reg_cpu->valor_CX[4] = '\0';
    }
    else if(!strcmp(registro_instr,"DX"))
    {
        strncpy(reg_cpu->valor_DX, valor, 4);
        reg_cpu->valor_DX[4] = '\0';
    }
    else if(!strcmp(registro_instr,"EAX"))
    {
        strncpy(reg_cpu->valor_EAX, valor, 8);
        reg_cpu->valor_EAX[8] = '\0';
    }
    else if(!strcmp(registro_instr,"EBX"))
    {
        strncpy(reg_cpu->valor_EBX, valor, 8);
        reg_cpu->valor_EBX[8] = '\0';
    }
    else if(!strcmp(registro_instr,"ECX"))
    {
        strncpy(reg_cpu->valor_ECX, valor, 8);
        reg_cpu->valor_ECX[8] = '\0';
    }
    else if(!strcmp(registro_instr,"EDX"))
    {
        strncpy(reg_cpu->valor_EDX, valor, 8);
        reg_cpu->valor_EDX[8] = '\0';
    }
    else if(!strcmp(registro_instr,"RAX"))
    {
        strncpy(reg_cpu->valor_RAX, valor, 16);
        reg_cpu->valor_RAX[16] = '\0';
    }
    else if(!strcmp(registro_instr,"RBX"))
    {
        strncpy(reg_cpu->valor_RBX, valor, 16);
        reg_cpu->valor_RBX[16] = '\0';
    }
    else if(!strcmp(registro_instr,"RCX"))
    {
        strncpy(reg_cpu->valor_RCX, valor, 16);
        reg_cpu->valor_RCX[16] = '\0';
    }
    else if(!strcmp(registro_instr,"RDX"))
    {
        strncpy(reg_cpu->valor_RDX, valor, 16);
        reg_cpu->valor_RDX[16] = '\0';

    }
    else
    {
        log_error(logger, "CPU: ERROR AL ASIGNAR REGISTRO, NOMBRE DESCONOCIDO");
    }
}
char *obtener_valor_registro(Registro_CPU registros_pcb, char *registro_buscado)
{
    char *valor = malloc(17); // solo lo inicializo, se tiene q pisar
    //quitar_salto_de_linea(registro_buscado);

    if (!strcmp(registro_buscado, "AX"))
    {
        strncpy(valor, registros_pcb.valor_AX, 4);
        valor[4] = '\0';
    }
    else if (!strcmp(registro_buscado, "BX"))
    {
        strncpy(valor, registros_pcb.valor_BX, 4);
        valor[4] = '\0';
    }
    else if (!strcmp(registro_buscado, "CX"))
    {
        strncpy(valor, registros_pcb.valor_CX, 4);
        valor[4] = '\0';
    }
    else if (!strcmp(registro_buscado, "DX"))
    {
        strncpy(valor, registros_pcb.valor_DX, 4);
        valor[4] = '\0';
    }
    else if (!strcmp(registro_buscado, "EAX"))
    {
        strncpy(valor, registros_pcb.valor_EAX, 8);
        valor[8] = '\0';
    }
    else if (!strcmp(registro_buscado, "EBX"))
    {
        strncpy(valor, registros_pcb.valor_EBX, 8);
        valor[8] = '\0';
    }
    else if (!strcmp(registro_buscado, "ECX"))
    {
        strncpy(valor, registros_pcb.valor_ECX, 8);
        valor[8] = '\0';
    }
    else if (!strcmp(registro_buscado, "EDX"))
    {
        strncpy(valor, registros_pcb.valor_EDX, 8);
        valor[8] = '\0';
    }
    else if (!strcmp(registro_buscado, "RAX"))
    {
        strncpy(valor, registros_pcb.valor_RAX, 16);
        valor[16] = '\0';
    }
    else if (!strcmp(registro_buscado, "RBX"))
    {
        strncpy(valor, registros_pcb.valor_RBX, 16);
        valor[16] = '\0';
    }
    else if (!strcmp(registro_buscado, "RCX"))
    {
        strncpy(valor, registros_pcb.valor_RCX, 16);
        valor[16] = '\0';
    }
    else if (!strcmp(registro_buscado, "RDX"))
    {
        strncpy(valor, registros_pcb.valor_RDX, 16);
        valor[16] = '\0';
    }
    else
    {
        log_error(logger, "CPU: ERROR AL BUSCAR REGISTRO, NOMBRE DESCONOCIDO");
        free(valor); // Liberar la memoria asignada
        return NULL;
    }
    return valor;
}

void ejecutar_set(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //OK
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <SET> - <REGISTRO:%s , VALOR: %s>",
                pcb->PID,
                instruccion->registro,
                instruccion->valor);
    asignar_a_registro(instruccion->valor, instruccion->registro, pcb);
    eliminar_paquete(paquete);
}

void ejecutar_mov_in(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <MOV_IN> - <REGISTRO:%s , DIRECCIÓN LÓGICA: %d>",
                pcb->PID,
                instruccion->registro,
                instruccion->direccionLogica
                );

    imprimir_registros(pcb->registros_cpu); //TODO BORRAR

    int32_t tamanio_registro = obtener_tamanio_registro(instruccion->registro);
    log_info(logger, "REGISTRO: <%s> - TAMANIO REGISTRO: <%d>", 
            instruccion->registro,
            tamanio_registro
            );
    if(tamanio_registro == 0) //NO LO ENCONTRÓ
        return;

    int32_t mov_in = MOV_IN; 
    agregar_a_paquete(paquete, &mov_in, sizeof(int32_t));
    agregar_a_paquete(paquete,&pcb->PID,sizeof(int32_t));
    agregar_a_paquete(paquete, &instruccion->direccionFisica, sizeof(int32_t));
    agregar_a_paquete(paquete,&tamanio_registro,sizeof(int32_t));
    enviar_paquete_a_servidor(paquete, socket_memoria);

    BUFFER *buffer;
    char* valor;
    switch (obtener_codigo_operacion(socket_memoria))
    {  
       case MOV_IN:
            log_info(logger, "[CPU]: RECIBI FIN DE MOV_IN ");
            buffer = recibir_buffer(socket_memoria);

            int tamanio;
            memcpy(&tamanio, buffer->stream, sizeof(int32_t));
            buffer->stream += sizeof(int32_t);
            valor = malloc(tamanio);

            memcpy(valor, buffer->stream, tamanio);
            buffer->stream += tamanio;

            valor[tamanio] = '\0';
        break;
        default:
            log_error(logger,"NO RECIBI FIN DE MOV_IN");
        break;
    }

    int num_segmento = floor(instruccion->direccionLogica / CPUConfig.TAM_MAX_SEGMENTO);

    log_warning(logger, "CPU: PID: <%d> - Acción: <LEER> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",
                pcb->PID,
                num_segmento,
                instruccion->direccionFisica,
                valor
                );
    asignar_a_registro(valor, instruccion->registro, pcb);
    eliminar_paquete(paquete);
    imprimir_registros(pcb->registros_cpu); //TODO BORRAR

    free(valor);
    free(buffer);
}

void ejecutar_mov_out(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //MODIFICADO CHK 4 PERO NO VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <MOV_OUT> - <DIRECCIÓN LÓGICA: %d, REGISTRO: %s>",
                pcb->PID,
                instruccion->direccionLogica,
                instruccion->registro
                );
    imprimir_registros(pcb->registros_cpu); //TODO BORRAR

    char *registro = instruccion->registro; //TODO BORRAR

    char *valor_registro = obtener_valor_registro(pcb->registros_cpu, registro);
    
    int32_t tamanio_registro = obtener_tamanio_registro(instruccion->registro);
    log_info(logger, "REGISTRO: <%s> - TAMANIO REGISTRO: <%d> - VALOR REGISTRO: %s", //TODO BORRAR
            instruccion->registro,
            tamanio_registro,
            valor_registro);
    if(tamanio_registro == 0) //NO LO ENCONTRÓ
        return;

    int32_t mov_out = MOV_OUT; 
    agregar_a_paquete(paquete, &mov_out, sizeof(int32_t));
    agregar_a_paquete(paquete,&pcb->PID,sizeof(int32_t));
    agregar_a_paquete(paquete, &instruccion->direccionFisica, sizeof(int32_t));
    agregar_a_paquete(paquete,&tamanio_registro,sizeof(int32_t));
    agregar_a_paquete(paquete, valor_registro, strlen(valor_registro));
    enviar_paquete_a_servidor(paquete, socket_memoria);

    log_info(logger, "//// CPU ENVIA PAQUETE A MEMORIA: TAMAÑO %d - VALOR %s ////", tamanio_registro, valor_registro);

    BUFFER *buffer;
    switch (obtener_codigo_operacion(socket_memoria))
    {  
       case MOV_OUT:
            log_info(logger, "[CPU]: RECIBI FIN DE MOV_OUT ");

            buffer = recibir_buffer(socket_memoria);
            int32_t un_pid;
            memcpy(&un_pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t)*2);
        break;
        default:
            log_error(logger, "[CPU]: NO RECIBI FIN DE MOV_OUT ");
        break;
    }

    int num_segmento = floor(instruccion->direccionLogica / CPUConfig.TAM_MAX_SEGMENTO);

    log_warning(logger, "CPU: PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",
                pcb->PID,
                num_segmento,
                instruccion->direccionFisica,
                valor_registro
                );
    eliminar_paquete(paquete);

    imprimir_registros(pcb->registros_cpu);

    free(buffer);
}

void ejecutar_IO(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //OK
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <I/O> - <TIEMPO: %d>",
                pcb->PID,
                instruccion->tiempo
                );
    PAQUETE *paquete2 = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete2, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete2, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete2, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete2, socket_kernel);
    eliminar_paquete(paquete2);
    eliminar_paquete(paquete);
}

void ejecutar_f_open(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) // NI MODIFICADO NI VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_OPEN> - <NOMBRE_ARCHIVO: %s>",
                pcb->PID,
                instruccion->nombreArchivo
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_close(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) // NI MODIFICADO NI VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_CLOSE> - <NOMBRE_ARCHIVO: %s>",
                pcb->PID,
                instruccion->nombreArchivo
                );
    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_seek(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) // NI MODIFICADO NI VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_SEEK> - <NOMBRE_ARCHIVO: %s> - <POSICION: %d> ",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->posicion
                );
    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_read(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) // NI MODIFICADO NI VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_READ> - <NOMBRE_ARCHIVO: %s> - <DIR_FISICA: %d> - <CANT_BYTES %d>",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->direccionFisica,
                instruccion->cantBytes
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    agregar_a_paquete(paquete_kernel, &instruccion->direccionFisica, sizeof(int32_t));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_write(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) // NI MODIFICADO NI VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_WRITE> - <NOMBRE_ARCHIVO: %s> - <DIR_FISICA: %d> - <CANT_BYTES %d>",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->direccionFisica,
                instruccion->cantBytes
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    agregar_a_paquete(paquete_kernel, &instruccion->direccionFisica, sizeof(int32_t));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_truncate(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) // NI MODIFICADO NI VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_TRUNCATE> - <NOMBRE_ARCHIVO: %s> - <TAM_ARCHIVO: %d>",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->tamanioArchivo
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_wait(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //OK
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <WAIT> - <RECURSO: %s>",
                pcb->PID,
                instruccion->recurso
                );
            
    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}
void ejecutar_signal(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //OK
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <SIGNAL> - <RECURSO: %s>",
                pcb->PID,
                instruccion->recurso
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_create_segment(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //MODIFICADO CHK 4 PERO NO VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <CREATE_SEGMENT> - <ID_SEGMENTO: %d> - <TAMANIO: %d>",
                pcb->PID,
                instruccion->idSegmento,
                instruccion->tamanioSegmento
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}
void ejecutar_delete_segment(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb) //MODIFICADO CHK 4 PERO NO VERIFICADO
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <DELETE_SEGMENT> - <ID_SEGMENTO: %d> ",
                pcb->PID,
                instruccion->idSegmento
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_yield(PAQUETE *paquete, PCB *pcb) //OK
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <YIELD>",
                pcb->PID
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_exit(PAQUETE *paquete, PCB *pcb) //OK
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <EXIT>",
                pcb->PID
                );

    PAQUETE *paquete_kernel = crear_paquete(PAQUETE_CPU);
    agregar_a_paquete(paquete_kernel, &pcb->PID, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->program_counter, sizeof(int32_t));
    agregar_a_paquete(paquete_kernel, &pcb->registros_cpu, sizeof(Registro_CPU));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    eliminar_paquete(paquete);
}

void imprimir_tabla_segmentos(t_list* tabla_segmentos){
    log_info(logger, "-------------------------------");
    for(int i = 0; i < list_size(tabla_segmentos); i++){
        SEGMENTO* segmento = (SEGMENTO*) list_get(tabla_segmentos, i);
        log_info(logger, "-> PID %d - ID %d - Base %d - Limite %d", segmento->pid, segmento->id, segmento->base, segmento->limite);
    }
    log_info(logger, "-------------------------------");
}

void imprimir_registros(Registro_CPU registro)
{
    log_info(logger, "--------------REGISTROS--------------");
    log_info(logger, "AX: %s", registro.valor_AX);
    log_info(logger, "BX: %s", registro.valor_BX);
    log_info(logger, "CX: %s", registro.valor_CX);
    log_info(logger, "DX: %s", registro.valor_DX);
    log_info(logger, "EAX: %s", registro.valor_EAX);
    log_info(logger, "EBX: %s", registro.valor_EBX);
    log_info(logger, "ECX: %s", registro.valor_ECX);
    log_info(logger, "EDX: %s", registro.valor_EDX);
    log_info(logger, "RAX: %s", registro.valor_RAX);
    log_info(logger, "RBX: %s", registro.valor_RBX);
    log_info(logger, "RCX: %s", registro.valor_RCX);
    log_info(logger, "RDX: %s", registro.valor_RDX);
    log_info(logger, "-------------------------------------");
}

void liberar_instruccion(Instruccion *instruccion)
{
    free(instruccion->nombreInstruccion);
    free(instruccion->valor);
    free(instruccion->registro);
    free(instruccion->nombreArchivo);
    free(instruccion->recurso);
    free(instruccion);
}