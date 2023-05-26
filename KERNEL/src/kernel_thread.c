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

        //TODO_A: RECIBIR INSTRUCCIONES DE CONSOLA
        case PAQUETE_2:
            // Recibir info consola
            manejar_proceso_consola();
            break;

        case INSTRUCCIONES:
             obtener_paquete_estructura_dinamica(socketConsola);        
            break;

        default:
            log_warning(logger, "[KERNEL]: Operacion desconocida desde consola.");
            break;
        }
    }
}

void manejar_proceso_consola()
{
    log_info(logger, "[KERNEL]: Creando PCB");

    PCB pcb;
    t_list *instrucciones = list_create();
    list_add(instrucciones, "Instruccion1");
    list_add(instrucciones, "Instruccion1");

    pcb.PID = PROCESO_ID++;
    pcb.instrucciones = instrucciones;
    pcb.program_counter = 1;

    log_info(logger, "[KERNEL]: PCB Creada: %d", pcb.PID);
    // pcb.registros_cpu;  //Tipo struct REGISTROS_CPU
    // pcb.tabla_segmentos; //Lista de Struct TABLA_SEGMENTOS
    // pcb.proxima_rafaga;
    // pcb.tiempo_ready;
    // pcb.archivos_abiertos; //Lista de struct ARCHIVOS_ABIERTOS

    // METER PCB A LISTA procesos
    // TODO_A: AVISAR QUE SE CREO EL PCB


    enviar_pcb_a_cpu(&pcb);
}

void enviar_pcb_a_cpu(PCB *pcb)
{

    log_info(logger, "[KERNEL]: Enviar PCB a CPU: preparando paquete");

    PAQUETE *paquete_pcb = crear_paquete(OP_PCB);

    log_info(logger, "[KERNEL]: Enviar PCB a CPU: serializar pcb %d", pcb->PID);

    paquete_pcb->buffer = serializar_pcb(pcb);

    log_info(logger, "[KERNEL]: Enviar PCB a CPU: enviar");

    enviar_paquete_a_servidor(paquete_pcb, socket_cpu);
}
