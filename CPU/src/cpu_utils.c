#include "cpu_utils.h"

// bool esExit = false;//posiblemente combinar ambos bool.
// bool esYield = false;

void iniciar_logger_cpu()
{
    logger = log_create(ARCHIVO_LOGGER, "CPU", 1, LOG_LEVEL_INFO);
    log_info(logger, "[CPU]: Logger creado correctamente");
}

void iniciar_config_cpu()
{
    config = config_create(ARCHIVO_CONFIG);
    rellenar_configuracion_cpu(config);
    log_info(logger, "[CPU]: Archivo Config creado y rellenado correctamente");
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

    if (socket_memoria < 0)
    {
        log_info(logger, "[CPU]: Error al conectar con memoria. Finalizando Ejecucion");
        log_error(logger, "[CPU]: memoria no está disponible");

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
    t_list *instrucciones = list_duplicate(pcb->instrucciones);

    Instruccion *prox_instruccion;

    int seguir = 1; // SI DEBE DEVOLVER CONTEXTO DE EJECUCIÓN ó SEG FAULT => CAMBIAR A 0.

    while (seguir)
    {
        prox_instruccion = list_get(instrucciones, pcb->program_counter); // FORMA PARTE DEL FETCH

        if (!decode_instruccion(prox_instruccion, pcb)) // SI SEGMENTATION FAULT
        {
            // avisar_seg_fault_kernel(pcb, prox_instruccion);
            // seguir = 0;
        }
        if (seguir)
        {
            ejecutar_instruccion(prox_instruccion, pcb);

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
    sleep(retardo);
}
bool requiere_traduccion(Instruccion *instruccion)
{
    char *nombre_instru = string_duplicate(instruccion->nombreInstruccion);

    if (!strcmp(nombre_instru, "MOV_IN") ||
        !strcmp(nombre_instru, "MOV_OUT") ||
        !strcmp(nombre_instru, "F_READ") ||
        !strcmp(nombre_instru, "F_WRITE"))
        return true;

    else
        return false;
}
int32_t realizar_traduccion(int32_t dir_logica, t_list *tabla_segmentos)
{
    int num_segmento = obtener_num_segmento(dir_logica);

    int desplazamiento_segmento = obtener_desplazamiento_segmento(dir_logica);

    SEGMENTO *segmento = (SEGMENTO *)list_get(tabla_segmentos, num_segmento);

    int32_t direccion_fisica = (segmento->base) + desplazamiento_segmento;

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
    int32_t tam_a_usar;
    if (!strcmp(Inst->nombreInstruccion, "MOV_IN"))
        tam_a_usar = strlen(Inst->registro);

    if (!strcmp(Inst->nombreInstruccion, "MOV_OUT"))
        tam_a_usar = strlen(Inst->registro);

    if (!strcmp(Inst->nombreInstruccion, "F_READ"))
        tam_a_usar = Inst->cantBytes;

    if (!strcmp(Inst->nombreInstruccion, "F_WRITE"))
        tam_a_usar = Inst->cantBytes;

    int num_segmento = obtener_num_segmento(dir_logica);

    int desplazamiento_segmento = obtener_desplazamiento_segmento(dir_logica);

    SEGMENTO *segmento = (SEGMENTO *)list_get(tabla_segmentos, num_segmento);
    int maximo = segmento->limite;

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
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

int ejecutar_instruccion(Instruccion *Instruccion, PCB *pcb) // EXECUTE //CADA INSTRUCCIÓN DEBE TENER SU log_warning(PID: <PID> - Ejecutando: <INSTRUCCION> - <PARAMETROS>)
{
    char *nombre_instru = string_duplicate(Instruccion->nombreInstruccion);
    PAQUETE *paquete = crear_paquete(INSTRUCCION);
    quitar_salto_de_linea(nombre_instru);

    if (!strcmp(nombre_instru, "SET"))
    {
        ejecutar_set(paquete, Instruccion, pcb);
        free(nombre_instru);
        return 1;
    }
    else if (!strcmp(nombre_instru, "MOV_IN"))
    {
        ejecutar_mov_in(paquete, Instruccion, pcb);
        free(nombre_instru);
        return 1;
    }
    else if (!strcmp(nombre_instru, "MOV_OUT"))
    {
        ejecutar_mov_out(paquete, Instruccion, pcb);
        free(nombre_instru);
        return 1;
    }
    else if (!strcmp(nombre_instru, "I/O"))
    {
        ejecutar_IO(paquete, Instruccion, pcb);
        free(nombre_instru);
        return 0;
    }
    else if (!strcmp(nombre_instru, "F_OPEN"))
    {
        ejecutar_f_open(paquete, Instruccion, pcb);
        free(nombre_instru);
        return 0;
    }
    else if (!strcmp(nombre_instru, "F_CLOSE"))
    {
        ejecutar_f_close(paquete, Instruccion, pcb);
        free(nombre_instru);
        return 0;
    }
    else if (!strcmp(nombre_instru, "F_SEEK"))
    {
        ejecutar_f_seek(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(nombre_instru, "F_READ"))
    {
        ejecutar_f_read(paquete, Instruccion, pcb); // DIR LOGICA?
        return 0;
    }
    else if (!strcmp(nombre_instru, "F_WRITE"))
    {
        ejecutar_f_write(paquete, Instruccion, pcb); // DIR LOGICA?
        return 0;
    }
    else if (!strcmp(nombre_instru, "F_TRUNCATE "))
    {
        ejecutar_f_truncate(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(nombre_instru, "WAIT")) // CAMIL
    {
        ejecutar_wait(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(nombre_instru, "SIGNAL")) // CAMIL
    {
        ejecutar_signal(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(nombre_instru, "CREATE_SEGMENT")) // CAMIL
    {
        ejecutar_create_segment(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(nombre_instru, "DELETE_SEGMENT ")) // CAMIL
    {
        ejecutar_delete_segment(paquete, Instruccion, pcb);
        return 0;
    }
    else if (!strcmp(nombre_instru, "YIELD"))
    {
        ejecutar_yield(paquete, pcb);
        free(nombre_instru);
        return 0;
    }
    else if (!strcmp(nombre_instru, "EXIT"))
    {
        ejecutar_exit(paquete, pcb);
        free(nombre_instru);
        return 0;
    }
    else
    {
        log_error(logger, "[CPU]: Codigo de Instruccion no encontrado");
        free(nombre_instru);
        eliminar_paquete(paquete);
        return 0;
    }
}

void asignar_a_registro(char *valor, char *registro_instr, PCB *pcb)
{
    Registro_CPU *reg_cpu = pcb->registros_cpu;

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
        log_error(logger, "CPU: ERROR AL ASIGNAR REGISTRO, NOMBRE DESCONOCIDO");
    }
}
char *obtener_valor_registro(Registro_CPU *registros_pcb, char *registro_buscado)
{
    char *valor = string_itoa(0); // solo lo inicializo, se tiene q pisar
    quitar_salto_de_linea(registro_buscado);

    if (!strcmp(registro_buscado, "AX"))
    {
        valor = string_duplicate(registros_pcb->valor_AX);
    }
    else if (!strcmp(registro_buscado, "BX"))
    {
        valor = string_duplicate(registros_pcb->valor_BX);
    }
    else if (!strcmp(registro_buscado, "CX"))
    {
        valor = string_duplicate(registros_pcb->valor_CX);
    }
    else if (!strcmp(registro_buscado, "DX"))
    {
        valor = string_duplicate(registros_pcb->valor_DX);
    }
    else if (!strcmp(registro_buscado, "EAX"))
    {
        valor = string_duplicate(registros_pcb->valor_EAX);
    }
    else if (!strcmp(registro_buscado, "EBX"))
    {
        valor = string_duplicate(registros_pcb->valor_EBX);
    }
    else if (!strcmp(registro_buscado, "ECX"))
    {
        valor = string_duplicate(registros_pcb->valor_ECX);
    }
    else if (!strcmp(registro_buscado, "EDX"))
    {
        valor = string_duplicate(registros_pcb->valor_EDX);
    }
    else if (!strcmp(registro_buscado, "RAX"))
    {
        valor = string_duplicate(registros_pcb->valor_RAX);
    }
    else if (!strcmp(registro_buscado, "RBX"))
    {
        valor = string_duplicate(registros_pcb->valor_RBX);
    }
    else if (!strcmp(registro_buscado, "RCX"))
    {
        valor = string_duplicate(registros_pcb->valor_RCX);
    }
    else if (!strcmp(registro_buscado, "RDX"))
    {
        valor = string_duplicate(registros_pcb->valor_RDX);
    }
    else
    {
        log_error(logger, "CPU: ERROR AL BUSCAR REGISTRO, NOMBRE DESCONOCIDO");
    }
    return valor;
}

void ejecutar_set(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
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
                instruccion->direccionLogica);
    agregar_a_paquete(paquete, MOV_IN, sizeof(int));
    agregar_a_paquete(paquete, instruccion->direccionFisica, sizeof(int32_t));
    enviar_paquete_a_servidor(paquete, socket_memoria);

    char *valor = string_duplicate(obtener_mensaje_del_servidor(socket_memoria));

    //... SE BLOQUEA HASTA QUE RESPONDA

    int num_segmento = floor(instruccion->direccionLogica / CPUConfig.TAM_MAX_SEGMENTO);

    log_warning(logger, "CPU: PID: <%d> - Acción: <LEER> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",
                pcb->PID,
                num_segmento,
                instruccion->direccionFisica,
                valor);
    asignar_a_registro(valor, instruccion->registro, pcb);
    eliminar_paquete(paquete);
}

void ejecutar_mov_out(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <MOV_OUT> - <DIRECCIÓN LÓGICA: %d, REGISTRO: %s>",
                pcb->PID,
                instruccion->direccionLogica,
                instruccion->registro);
    char *registro = string_duplicate(instruccion->registro);
    char *valor_registro = string_duplicate(obtener_valor_registro(pcb->registros_cpu, registro));

    // agregar_a_paquete(paquete, 3, sizeof(int));
    CODIGO_INSTRUCCION mov_out = MOV_OUT;
    agregar_a_paquete(paquete, &mov_out, sizeof(CODIGO_INSTRUCCION));
    agregar_a_paquete(paquete, &(instruccion->direccionFisica), sizeof(int32_t));
    agregar_a_paquete(paquete, &valor_registro, strlen(valor_registro) + 1);
    enviar_paquete_a_servidor(paquete, socket_memoria);
    char *mensaje = obtener_mensaje_del_servidor(socket_memoria);

    //... SE BLOQUEA HASTA QUE RESPONDA

    log_info(logger, "CPU: Recibi un mensaje de MEMORIA como RTA a MOV_OUT: <%s>", mensaje);
    int num_segmento = floor(instruccion->direccionLogica / CPUConfig.TAM_MAX_SEGMENTO);

    log_warning(logger, "CPU: PID: <%d> - Acción: <ESCRIBIR> - Segmento: <%d> - Dirección Física: <%d> - Valor: <%s>",
                pcb->PID,
                num_segmento,
                instruccion->direccionFisica,
                valor_registro);
    eliminar_paquete(paquete);
}

void ejecutar_IO(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <I/O> - <TIEMPO: %d>",
                pcb->PID,
                instruccion->tiempo);
    enviar_pcb(pcb);
    // agregar_a_paquete(paquete,IO,sizeof(int));
    CODIGO_INSTRUCCION io = IO;
    agregar_a_paquete(paquete, &io, sizeof(CODIGO_INSTRUCCION));
    agregar_a_paquete(paquete, &instruccion->tiempo, sizeof(int32_t));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_open(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_OPEN> - <NOMBRE_ARCHIVO: %s>",
                pcb->PID,
                instruccion->nombreArchivo);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION f_open = F_OPEN;
    agregar_a_paquete(paquete, &f_open, sizeof(int));
    agregar_a_paquete(paquete, &instruccion->nombreArchivo, strlen(instruccion->nombreArchivo) + 1);
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_close(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_CLOSE> - <NOMBRE_ARCHIVO: %s>",
                pcb->PID,
                instruccion->nombreArchivo);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION f_close = F_CLOSE;
    agregar_a_paquete(paquete, &f_close, sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->nombreArchivo), strlen(instruccion->nombreArchivo) + 1);
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_seek(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_SEEK> - <NOMBRE_ARCHIVO: %s> - <POSICION: %d> ",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->posicion);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION f_seek = F_SEEK;
    agregar_a_paquete(paquete, &f_seek, sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->nombreArchivo), strlen(instruccion->nombreArchivo) + 1);
    agregar_a_paquete(paquete, &(instruccion->posicion), sizeof(int));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_read(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_READ> - <NOMBRE_ARCHIVO: %s> - <DIR_FISICA: %d> - <CANT_BYTES %d>",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->direccionFisica,
                instruccion->cantBytes);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION f_read = F_READ;
    agregar_a_paquete(paquete, &f_read, sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->nombreArchivo), strlen(instruccion->nombreArchivo) + 1);
    agregar_a_paquete(paquete, &(instruccion->direccionFisica), sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->cantBytes), sizeof(int));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_write(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_WRITE> - <NOMBRE_ARCHIVO: %s> - <DIR_FISICA: %d> - <CANT_BYTES %d>",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->direccionFisica,
                instruccion->cantBytes);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION f_write = F_WRITE;
    agregar_a_paquete(paquete, &f_write, sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->nombreArchivo), strlen(instruccion->nombreArchivo) + 1);
    agregar_a_paquete(paquete, &(instruccion->direccionFisica), sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->cantBytes), sizeof(int));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_f_truncate(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <F_TRUNCATE> - <NOMBRE_ARCHIVO: %s> - <TAM_ARCHIVO: %d>",
                pcb->PID,
                instruccion->nombreArchivo,
                instruccion->tamanioArchivo);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION f_truncate = F_TRUNCATE;
    agregar_a_paquete(paquete, &f_truncate, sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->nombreArchivo), strlen(instruccion->nombreArchivo) + 1);
    agregar_a_paquete(paquete, &(instruccion->tamanioArchivo), sizeof(int));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_wait(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <WAIT> - <RECURSO: %s>",
                pcb->PID,
                instruccion->recurso);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION wait = WAIT;
    agregar_a_paquete(paquete, &wait, sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->recurso), strlen(instruccion->recurso) + 1);
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}
void ejecutar_signal(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <SIGNAL> - <RECURSO: %s>",
                pcb->PID,
                instruccion->recurso);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION signal = SIGNAL;
    agregar_a_paquete(paquete, &signal, sizeof(CODIGO_INSTRUCCION));
    agregar_a_paquete(paquete, &(instruccion->recurso), strlen(instruccion->recurso) + 1);
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_create_segment(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <CREATE_SEGMENT> - <ID_SEGMENTO: %d> - <TAMANIO: %d>",
                pcb->PID,
                instruccion->idSegmento,
                instruccion->tamanioSegmento);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION create_segment = CREATE_SEGMENT;
    agregar_a_paquete(paquete, &create_segment, sizeof(CODIGO_INSTRUCCION));
    agregar_a_paquete(paquete, &(instruccion->idSegmento), sizeof(int));
    agregar_a_paquete(paquete, &(instruccion->tamanioSegmento), sizeof(int));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}
void ejecutar_delete_segment(PAQUETE *paquete, Instruccion *instruccion, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <DELETE_SEGMENT> - <ID_SEGMENTO: %d> ",
                pcb->PID,
                instruccion->idSegmento);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION delete_segment = DELETE_SEGMENT;
    agregar_a_paquete(paquete, &delete_segment, sizeof(CODIGO_INSTRUCCION));
    agregar_a_paquete(paquete, &(instruccion->idSegmento), sizeof(int));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void ejecutar_yield(PAQUETE *paquete, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <YIELD>",
                pcb->PID);
    enviar_mensaje_a_cliente("YIELD", socket_kernel);
    enviar_pcb(pcb);
    //CODIGO_INSTRUCCION yield = YIELD;
    char* yield = "YIELD";
    //agregar_a_paquete(paquete, &yield, strlen(yield)+1);
    //enviar_paquete_a_cliente(paquete, socket_kernel);
    
    eliminar_paquete(paquete);
}

void ejecutar_exit(PAQUETE *paquete, PCB *pcb)
{
    log_warning(logger, "CPU: PID: <%d> - Ejecutando: <EXIT>",
                pcb->PID);
    enviar_pcb(pcb);
    CODIGO_INSTRUCCION exit = EXIT;
    agregar_a_paquete(paquete, &exit, sizeof(CODIGO_INSTRUCCION));
    enviar_paquete_a_cliente(paquete, socket_kernel);
    eliminar_paquete(paquete);
}

void quitar_salto_de_linea(char *cadena) {
    int longitud = strcspn(cadena, "\n");
    cadena[longitud] = '\0';
}