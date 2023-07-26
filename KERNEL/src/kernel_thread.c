#include "kernel_thread.h"

int PROCESO_ID = 4000;

void esperar_consola(int socketKernel)
{
    while (true)
    {

        log_info(logger, "[KERNEL]: Esperando conexiones de Consola...");

        int socketConsola = esperar_cliente(socketKernel);

        if (socketConsola < 0)
        {
            log_warning(logger, "[KERNEL]: Consola desconectada.");

            return;
        }

        log_info(logger, "[KERNEL]: Conexión de Consola establecida.");

        Hilo hilo_consola;
        pthread_create(&hilo_consola, NULL, (void *)manejar_paquete_consola, (void *)socketConsola);
        pthread_detach(hilo_consola);

        
    }
}

void manejar_paquete_consola(int socketConsola)
{
    int recibi_instrucciones = 0;

    while (true)
    {
        char *mensaje;
        switch (obtener_codigo_operacion(socketConsola))
        {
        case MENSAJE:
            mensaje = obtener_mensaje_del_cliente(socketConsola);
            log_info(logger, "[KERNEL]: Mensaje recibido de Consola: %s", mensaje);
            // TODO_A: mandar un mensaje a socket_consola y deberia llegarle solamente a la instancia de consola que corresponda
            free(mensaje);
            break;

        case DESCONEXION:
            log_warning(logger, "[KERNEL]: Conexión de Consola terminada.");
            return;
        case INSTRUCCIONES:
            t_list *instrucciones = obtener_paquete_estructura_dinamica(socketConsola);
            confirmar_recepcion_a_consola(socketConsola);
            manejar_proceso_consola(instrucciones, socketConsola);
            recibi_instrucciones = 1;
            break;

        default:
            log_warning(logger, "[KERNEL]: Operacion desconocida desde consola.");
            break;
        }

        if(recibi_instrucciones){
            log_info(logger, "[KERNEL]: Recibi las instrucciones. Muere el hilo de la consola <%d>.", socketConsola);
            break;
        }

    }
    
}

void manejar_proceso_consola(t_list *instrucciones, int socket_consola)
{
    //log_info(logger, "[KERNEL]: Creando PCB");
    
    pthread_mutex_lock(&mx_procesos);
    
    PCB *pcb = malloc(sizeof(PCB));

    pcb->PID = PROCESO_ID++;
    pcb->instrucciones = instrucciones;
    pcb->program_counter = 0;

    pcb->socket_consola = socket_consola;

    Proceso *proceso = malloc(sizeof(Proceso));

    pcb->estimacion_cpu_proxima_rafaga = atoi(KernelConfig.ESTIMACION_INICIAL);

    proceso->estado = NEW;
    proceso->pcb = pcb;
    proceso->pcb->estimacion_cpu_proxima_rafaga = atoi(KernelConfig.ESTIMACION_INICIAL);
    proceso->pcb->estimacion_cpu_anterior = 0;

    proceso->pcb->archivos_abiertos = list_create();
    
    proceso->pcb->tabla_segmentos = list_create();

    proceso->pcb->recursos_asignados = list_create();

    log_warning(logger, "[KERNEL]: Proceso Creado (en NEW) - PID: <%d>", pcb->PID);
    
    bool en_new(Proceso * proceso)
    {
        return proceso->estado == NEW;
    }

    
    t_list *procesos_en_new = list_filter(procesos, (void *)en_new);
    
    list_add(procesos, proceso);
    pthread_mutex_unlock(&mx_procesos);

    sem_post(&semaforo_new);

    if (list_size(procesos_en_new) == 0 && (list_size(procesos)-1)!=0)
    {
        sem_post(&semaforo_planificador);

        bool en_finished(Proceso * proceso)
        {
            return proceso->estado == FINISHED;
        }

    
        t_list *procesos_finished = list_filter(procesos, (void *)en_finished);
        //log_info(logger, "finished %d - total %d", list_size(procesos_finished), list_size(procesos));
        if(list_size(procesos_finished) == list_size(procesos)-1){ // procesos - 1 porque ese es el new que recien se creo
            //log_info(logger, "destrabar semaforo ejecutando");
            sem_post(&semaforo_ejecutando);
        }
    }

    
}

void confirmar_recepcion_a_consola(int socket_consola){
    log_info(logger, "[KERNEL]: Confirmando recepcion a consola - SOCKET_CONSOLA: <%d>", socket_consola);
    PAQUETE *paquete = crear_paquete(RECEPCION_OK);
    enviar_paquete_a_cliente(paquete, socket_consola);
}

void enviar_pcb_a_cpu(PCB *pcb)
{
    PAQUETE *paquete_pcb = crear_paquete(OP_PCB);
    
    paquete_pcb->buffer = serializar_pcb(pcb);

    Instruccion* instruccion_enviar = list_get(pcb->instrucciones, pcb->program_counter);
    log_warning(logger, "[KERNEL]: Enviando PCB <%d> a CPU - Instruccion: %s", pcb->PID, instruccion_enviar->nombreInstruccion);

    enviar_paquete_a_servidor(paquete_pcb, socket_cpu);

    eliminar_paquete(paquete_pcb);
}
