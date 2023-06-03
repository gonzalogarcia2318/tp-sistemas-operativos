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

            // Para testear . Borrar.
            for (int i = 0; i < list_size(instrucciones); i++)
            {
                Instruccion *instruccion = list_get(instrucciones, i);
                log_info(logger, "Instruccion %d: nombre: %s", i, instruccion->nombreInstruccion);
            }

            manejar_proceso_consola(instrucciones);

            break;

        default:
            log_warning(logger, "[KERNEL]: Operacion desconocida desde consola.");
            break;
        }
    }
}

void manejar_proceso_consola(t_list *instrucciones)
{
    log_info(logger, "[KERNEL]: Creando PCB");

    PCB *pcb = malloc(sizeof(PCB));

    pcb->PID = PROCESO_ID++;
    pcb->instrucciones = instrucciones;
    pcb->program_counter = 0;
    
   

    log_info(logger, "[KERNEL]: PCB Creada - Proceso - PID: <%d>", pcb->PID);

    Proceso *proceso = malloc(sizeof(Proceso));
    SEGMENTO * segmento0 = malloc (sizeof(SEGMENTO));

    segmento0->base = 0;
    segmento0->id =0;
    segmento0->limite=100;

    proceso->estado = NEW;
    proceso->pcb = pcb;

    proceso->pcb->registros_cpu = malloc(112);
    
    proceso->pcb->tabla_segmentos = list_create();
    list_add(proceso->pcb->tabla_segmentos, segmento0);
    
    bool en_new(Proceso * proceso)
    {
        return proceso->estado == NEW;
    }

    pthread_mutex_lock(&mx_procesos);
    t_list *procesos_en_new = list_filter(procesos, (void *)en_new);
    
    list_add(procesos, proceso);
    pthread_mutex_unlock(&mx_procesos);

    sem_post(&semaforo_new);

    if (list_size(procesos_en_new) == 0 && (list_size(procesos)-1)!=0)
    {
        sem_post(&semaforo_planificador);
    }
}

void enviar_pcb_a_cpu(PCB *pcb)
{
    log_info(logger, "[KERNEL]: Enviar PCB a CPU: preparando paquete");

    PAQUETE *paquete_pcb = crear_paquete(OP_PCB);

    log_info(logger, "[KERNEL]: Enviar PCB a CPU: serializar pcb %d", pcb->PID);

    paquete_pcb->buffer = serializar_pcb(pcb);

    log_info(logger, "[KERNEL]: Enviar PCB a CPU: enviar");

    enviar_paquete_a_servidor(paquete_pcb, socket_cpu);

    eliminar_paquete(paquete_pcb);
}
