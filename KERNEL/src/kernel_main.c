#include "kernel_utils.h"

// VARIABLE COMPARTIDA: procesos -> [{pcb: 1, estado: new}, {pcb: 2, estado: new}]

t_queue *cola_ready;
t_queue *cola_io;

sem_t semaforo_planificador;
sem_t semaforo_ejecutando;

sem_t semaforo_multiprogramacion;


sem_t semaforo_new;
sem_t semaforo_io;

pthread_mutex_t mx_procesos;

t_list *procesos;
t_list *recursos;

t_list * listaReady;

void manejar_paquete_cpu();
void manejar_hilo_io();

void cambiar_estado(Proceso *proceso, ESTADO estado);
void actualizar_pcb(Proceso *proceso, PCB *pcb);
Instruccion *buscar_instruccion_por_counter(Proceso *proceso, PCB *pcb);
t_queue* calcular_lista_ready_HRRN (t_queue * cola_ready);

void manejar_io(Proceso *proceso, int32_t PID, int tiempo);
void manejar_wait(Proceso *proceso, char *nombre_recurso);
void manejar_signal(Proceso *proceso, char *nombre_recurso);
void manejar_yield(Proceso *proceso, PCB *pcb);
void manejar_exit(Proceso *proceso, PCB *pcb);

void quitar_salto_de_linea(char *cadena);
CODIGO_INSTRUCCION obtener_codigo_instruccion_numero(char *instruccion);

int main(int argc, char** argv)
{

    // TODO: Destruir la lista al final.
    procesos = list_create();
    cola_ready = queue_create();
    cola_io = queue_create();

    sem_init(&semaforo_new, 0, 0);
    pthread_mutex_init(&mx_procesos, NULL);
    sem_init(&semaforo_io, 0, 0);

    char* path_config = argv[1];

    iniciar_logger_kernel();

    if(iniciar_config_kernel(path_config) == FAILURE)
        return EXIT_FAILURE;

    recursos = crear_recursos(KernelConfig.RECURSOS, KernelConfig.INSTANCIAS_RECURSOS);

    if (iniciar_servidor_kernel() == SUCCESS)
    {
        if(conectar_con_memoria() == FAILURE)
            return EXIT_FAILURE;

        if(conectar_con_file_system() == FAILURE)
            return EXIT_FAILURE; 

        if(conectar_con_cpu() == FAILURE)
            return EXIT_FAILURE;

        conectar_con_consola();
    }

    Hilo hilo_cpu;
    pthread_create(&hilo_cpu, NULL, (void *)manejar_paquete_cpu, NULL);
    pthread_detach(hilo_cpu);

    Hilo hilo_io;
    pthread_create(&hilo_io, NULL, (void *)manejar_hilo_io, NULL);
    pthread_detach(hilo_io);

    // manejar_proceso_consola();

    sem_init(&semaforo_planificador, 0, 1);

    sem_init(&semaforo_ejecutando, 0, 1);

    // grado multiprogramacion
    sem_init(&semaforo_multiprogramacion, 0, atoi(KernelConfig.GRADO_MAX_MULTIPROGRAMACION));

    sem_wait(&semaforo_new); // Para que no empiece sin que no haya ningun proceso

    
    while (true)
    {

        sem_wait(&semaforo_planificador);

        bool en_new(Proceso * proceso)
        {
            return proceso->estado == NEW;
        }

        pthread_mutex_lock(&mx_procesos);
        t_list *procesos_en_new = list_filter(procesos, (void *)en_new);

        log_info(logger, "En NEW: %d", list_size(procesos_en_new));
        if (list_size(procesos_en_new) > 0)
        {
            sem_wait(&semaforo_multiprogramacion);
            Proceso *proceso_para_ready = (Proceso *)list_get(procesos_en_new, 0);

            cambiar_estado(proceso_para_ready,READY);
            proceso_para_ready->pcb->tiempo_ready = time(NULL); 
            queue_push(cola_ready, (Proceso *)proceso_para_ready);

            log_info(logger, "Proceso %d -> READY", (proceso_para_ready->pcb)->PID);
            
        }
        pthread_mutex_unlock(&mx_procesos);

        sem_wait(&semaforo_ejecutando); // Ejecuta uno a la vez

        log_info(logger, "En ready: %d", queue_size(cola_ready));
        if (!queue_is_empty(cola_ready))
        {

            if(strcmp(KernelConfig.ALGORITMO_PLANIFICACION,"HRRN")==0){

                 //Verificar Cuenta HRRRN - Quien pasa a ejecutar?
                cola_ready = calcular_lista_ready_HRRN(cola_ready);
                
            }
            
            Proceso *proceso_a_ejecutar = (Proceso *)queue_pop(cola_ready);
            cambiar_estado(proceso_a_ejecutar,EXEC);
   
            // destrabar ready
            sem_post(&semaforo_planificador);
            log_info(logger, "Proceso %d -> EXEC", (proceso_a_ejecutar->pcb)->PID);

            //   Enviar PCB a CPU
            log_info(logger, "Enviando %d - con program counter = %d", (proceso_a_ejecutar->pcb)->PID, (proceso_a_ejecutar->pcb)->program_counter);
            
            enviar_pcb_a_cpu(proceso_a_ejecutar->pcb);
            proceso_a_ejecutar->pcb->tiempo_cpu_real_inicial = time(NULL);
        }
        else
        {
            sem_post(&semaforo_ejecutando);
        }
    }

    terminar_ejecucion();

    return EXIT_SUCCESS;
}

void manejar_paquete_cpu()
{
    while (true)
    {
        char *mensaje;
        switch (obtener_codigo_operacion(socket_cpu))
        {
        case MENSAJE:
            mensaje = obtener_mensaje_del_cliente(socket_cpu);
            log_info(logger, "[KERNEL]: Mensaje recibido de CPU: %s", mensaje);
            free(mensaje);
            break;
        case DESCONEXION:
            log_warning(logger, "[KERNEL]: Conexión de CPU terminada.");
            return;

        case PAQUETE_CPU:
            log_info(logger, "[KERNEL] Llego PCB");

            BUFFER *buffer = recibir_buffer(socket_cpu);

            PCB *pcb = malloc(sizeof(PCB));

            // Sumamos sizeof(int32_t) porque en PAQUETE se manda SIEMPRE [tamaño-valor]
            memcpy(&(pcb->PID), buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor
            memcpy(&(pcb->program_counter), buffer->stream + sizeof(int32_t), sizeof(int32_t));
            buffer->stream += (sizeof(int32_t) * 2);

            Proceso *proceso = obtener_proceso_por_pid(pcb->PID);

            actualizar_pcb(proceso, pcb);

            Instruccion *instruccion = buscar_instruccion_por_counter(proceso, pcb);

            // Para mandar a memoria o file system?
            PAQUETE *paquete_instruccion = crear_paquete(INSTRUCCION);

            switch (obtener_codigo_instruccion_numero(instruccion->nombreInstruccion))
            {
            case IO:
                // EJEMPLO PARA OBTENER UN DATO QUE VIENE DE CPU DESPUES DE PID Y PROGRAM COUNTER
                // En IO no hace falta porque ya tenemos el tiempo guardado, sirve como ejemplo
                // int32_t tiempo = 0;
                // memcpy(&(tiempo), buffer->stream+sizeof(int32_t), sizeof(int32_t));
                // buffer->stream += (sizeof(int32_t)*2);
                log_info(logger, "[KERNEL] Llego Instruccion IO");
                manejar_io(proceso, pcb->PID, instruccion->tiempo);
                break;

            case WAIT:
                log_info(logger, "[KERNEL] Llego Instruccion WAIT  - %d", proceso->pcb->PID);
                manejar_wait(proceso, instruccion->recurso);
                break;

            case SIGNAL:
                log_info(logger, "[KERNEL] Llego Instruccion SIGNAL - %d", proceso->pcb->PID);
                manejar_signal(proceso, instruccion->recurso);
                break;

            case YIELD:
                log_info(logger, "[KERNEL] Llego Instruccion YIELD");

                memcpy(&(pcb->registros_cpu), buffer->stream + sizeof(int32_t), sizeof(Registro_CPU));
                buffer->stream += (sizeof(int32_t) + sizeof(Registro_CPU));

                manejar_yield(proceso, pcb);
                break;

            case EXIT:
                log_info(logger, "[KERNEL] Llego Instruccion EXIT");

                memcpy(&(pcb->registros_cpu), buffer->stream + sizeof(int32_t), sizeof(Registro_CPU));
                buffer->stream += (sizeof(int32_t) + sizeof(Registro_CPU));

                manejar_exit(proceso, pcb);
                break;

            case F_OPEN:
                log_info(logger, "[KERNEL] Llego Instruccion F_OPEN - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, instruccion->nombreArchivo);

                // HACER

                // CODIGO_INSTRUCCION* f_open = F_OPEN;
                // agregar_a_paquete(paquete_instruccion,&f_open,sizeof(CODIGO_INSTRUCCION));
                // enviar_paquete_a_servidor(paquete, socket_file_system);
                // char *mensaje = obtener_mensaje_del_servidor(socket_file_system);
                // log_info(logger, "KERNEL: Recibi un mensaje de FS con motivo de F_OPEN:%s", mensaje);
                //
                break;

            case F_CLOSE:
                log_info(logger, "[KERNEL] Llego Instruccion F_CLOSE - Proceso PID:<%d> - Archivo: <%s>", proceso->pcb->PID, instruccion->nombreArchivo);
                // HACER
                //
                break;

            case F_SEEK:
                log_info(logger, "[KERNEL] Llego Instruccion F_SEEK");
                log_info(logger, "Parametros: %s - %d", instruccion->nombreArchivo, instruccion->posicion);
                // HACER
                //
                break;

            case F_READ:
                log_info(logger, "[KERNEL] Llego Instruccion F_READ");

                // Direccion fisica la manda CPU
                memcpy(&(instruccion->direccionFisica), buffer->stream + sizeof(int32_t), sizeof(int32_t));
                buffer->stream += (sizeof(int32_t) * 2);

                log_info(logger, "Parametros: %s - %d - %d", instruccion->nombreArchivo, instruccion->direccionFisica, instruccion->cantBytes);
                // HACER
                //
                break;

            case F_WRITE:
                log_info(logger, "[KERNEL] Llego Instruccion F_WRITE");

                // Direccion fisica la manda CPU
                memcpy(&(instruccion->direccionFisica), buffer->stream + sizeof(int32_t), sizeof(int32_t));
                buffer->stream += (sizeof(int32_t) * 2);

                log_info(logger, "Parametros: %s - %d - %d", instruccion->nombreArchivo, instruccion->direccionFisica, instruccion->cantBytes);
                // HACER
                //
                break;

            case F_TRUNCATE:
                log_info(logger, "[KERNEL] Llego Instruccion F_TRUNCATE");
                log_info(logger, "Parametros: %s - %d", instruccion->nombreArchivo, instruccion->tamanioArchivo);
                // HACER
                //
                break;

            case CREATE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion CREATE_SEGMENT");
                log_info(logger, "Parametros: %d - %d", instruccion->idSegmento, instruccion->tamanioSegmento);
                // HACER
                //
                break;

            case DELETE_SEGMENT:
                log_info(logger, "[KERNEL] Llego Instruccion DELETE_SEGMENT");
                log_info(logger, "Parametros: %d", instruccion->idSegmento);
                // HACER
                //
                break;

            default:
                log_warning(logger, "[KERNEL]: Operacion desconocida desde CPU.");
                break;
            }

            // Ya volvio el proceso de la CPU -> pasamos a ejecutar otro
            sem_post(&semaforo_ejecutando);

            break;
        }
    }
}

Proceso *obtener_proceso_por_pid(int32_t PID)
{
    bool comparar_pcb_ids(Proceso * proceso)
    {
        return proceso->pcb->PID == PID;
    }
    Proceso *proceso = list_find(procesos, (void *)comparar_pcb_ids);
    return proceso;
}

double calcular_response_ratio (double tiempo_esperado_ready ,double tiempo_en_cpu){

return ((tiempo_esperado_ready + tiempo_en_cpu) / tiempo_en_cpu);

}

double calcular_estimacion_cpu (PCB * pcb){

    return pcb->estimacion_cpu_anterior * atoi((KernelConfig.HRRN_ALFA)) + pcb->tiempo_cpu_real * (1 - atoi(KernelConfig.HRRN_ALFA));

}

void actualizar_pcb(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos); // proceso* esta en lista compartida procesos
    // Proceso *proceso = obtener_proceso_por_pid(pcb->PID);
    proceso->pcb->program_counter = pcb->program_counter;
    // algun dato mas? Si
    proceso->pcb->tiempo_cpu_real = difftime(time(NULL),proceso->pcb->tiempo_cpu_real_inicial);
    proceso->pcb->estimacion_cpu_anterior = proceso->pcb->estimacion_cpu_proxima_rafaga;
    pthread_mutex_unlock(&mx_procesos);
}

void actualizar_registros(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos); // proceso* esta en lista compartida procesos
    // Proceso *proceso = obtener_proceso_por_pid(pcb->PID);
    proceso->pcb->registros_cpu = pcb->registros_cpu;
    // algun dato mas?
    pthread_mutex_unlock(&mx_procesos);
}

t_queue* calcular_lista_ready_HRRN (t_queue * cola_ready){

    Proceso * proceso = malloc(sizeof(Proceso));
    listaReady = list_create();

    while(!queue_is_empty(cola_ready)){

        proceso = queue_pop(cola_ready);

        if(proceso->pcb->estimacion_cpu_anterior !=0){

            proceso->pcb->estimacion_cpu_proxima_rafaga = calcular_estimacion_cpu(proceso->pcb);


        }
        

        proceso->pcb->response_Ratio = calcular_response_ratio(proceso->pcb->tiempo_ready, proceso->pcb->estimacion_cpu_proxima_rafaga );

        list_add(listaReady,proceso);


    }

     bool mayor_rr(Proceso * proceso1 , Proceso * proceso2)
    {
        return proceso1->pcb->response_Ratio > proceso2->pcb->response_Ratio;
    }

    list_sort(listaReady,(void *)mayor_rr);

    for(int i=0; i<list_size(listaReady); i++){
        queue_push(cola_ready,list_get(listaReady,i));

        log_warning(logger,"Proceso en cola : %d", ((Proceso *)list_get(listaReady,i))->pcb->PID);
        log_warning(logger,"Con RR : %f", ((Proceso *)list_get(listaReady,i))->pcb->response_Ratio);
    }

    list_destroy(listaReady);



    return cola_ready;

}



// duda por lo que dice el enunciado
// asi esta bien o 1 hilo por proceso y que sleep de ese hilo?
void manejar_hilo_io()
{
    while (true)
    {
        sem_wait(&semaforo_io);
        Proceso_IO *proceso_io = (Proceso_IO *)queue_pop(cola_io);
        sleep(proceso_io->tiempo_bloqueado);

        Proceso *proceso = obtener_proceso_por_pid(proceso_io->PID);

        log_error(logger, "[KERNEL] poner ready %d ", proceso->pcb->PID);
        cambiar_estado(proceso, READY);

        proceso->pcb->tiempo_ready = time(NULL); 

        sem_post(&semaforo_planificador);
        //sem_post(&semaforo_ejecutando);

        queue_push(cola_ready, proceso);
    }
}

void manejar_wait(Proceso *proceso, char *nombre_recurso)
{
    quitar_salto_de_linea(nombre_recurso);

    bool comparar_recurso_por_nombre(Recurso * recurso)
    {
        return strcmp(recurso->nombre, nombre_recurso) == 0;
    }

    Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);

    if (recurso == NULL)
    {
        cambiar_estado(proceso, FINISHED);
        return;
    }

    if (recurso->instancias > 0)
    {
        log_info(logger, "[KERNEL] Descontar recurso %s - %d", nombre_recurso, recurso->instancias);
        recurso->instancias -= 1;

        proceso->pcb->program_counter++;
        cambiar_estado(proceso, READY); 
        proceso->pcb->tiempo_ready = time(NULL);      
        queue_push(cola_ready, proceso);
        
    }
    else
    {
        cambiar_estado(proceso, BLOCK);

        queue_push(recurso->cola_block, proceso);
    }
}

void manejar_signal(Proceso *proceso, char *nombre_recurso)
{
    quitar_salto_de_linea(nombre_recurso);
    bool comparar_recurso_por_nombre(Recurso * recurso)
    {
        return strcmp(recurso->nombre, nombre_recurso) == 0;
    }

    Recurso *recurso = list_find(recursos, (void *)comparar_recurso_por_nombre);

    if (recurso == NULL)
    {
        cambiar_estado(proceso, FINISHED);
        return;
    }

    log_info(logger, "[KERNEL] Sumar recurso %s - %d", nombre_recurso, recurso->instancias);
    recurso->instancias += 1;

    if (!queue_is_empty(recurso->cola_block))
    {
        log_info(logger, "[KERNEL] desbloquear %s ", nombre_recurso);
        Proceso *proceso_bloqueado = (Proceso *)queue_pop(recurso->cola_block);
        // estado es EXEC? hay que sacarlo de block
        proceso_bloqueado->estado = EXEC;
    }

    proceso->pcb->program_counter++;
    cambiar_estado(proceso, READY);
    proceso->pcb->tiempo_ready = time(NULL);   
    queue_push(cola_ready, proceso);
}

Instruccion *buscar_instruccion_por_counter(Proceso *proceso, PCB *pcb)
{
    Instruccion *instruccion = list_get(proceso->pcb->instrucciones, pcb->program_counter);

    return instruccion;
}

void manejar_yield(Proceso *proceso, PCB *pcb)
{
    pthread_mutex_lock(&mx_procesos);

    proceso->pcb->program_counter++;

    proceso->pcb->registros_cpu = pcb->registros_cpu;

    log_error(logger, "[KERNEL] poner en ready POR YIELD %d", proceso->pcb->PID);
    cambiar_estado(proceso, READY);
    proceso->pcb->tiempo_ready = time(NULL);   
    queue_push(cola_ready, proceso);

    pthread_mutex_unlock(&mx_procesos);
}

void manejar_exit(Proceso *proceso, PCB *pcb)
{
    proceso->pcb->registros_cpu = pcb->registros_cpu;

    cambiar_estado(proceso, FINISHED);
    sem_post(&semaforo_multiprogramacion);
    // liberar recursos asignados
    // avisar a memoria para que libere estructuras
    // avisar a consola que finalizo
}

void manejar_io(Proceso *proceso, int32_t PID, int tiempo)
{
    cambiar_estado(proceso, BLOCK);

    Proceso_IO *proceso_io = malloc(sizeof(Proceso_IO));
    proceso_io->PID = PID;
    proceso_io->tiempo_bloqueado = tiempo;

    proceso->pcb->program_counter++;


    queue_push(cola_io, proceso_io);

    sem_post(&semaforo_io);
}

CODIGO_INSTRUCCION obtener_codigo_instruccion_numero(char *instruccion)
{
    quitar_salto_de_linea(instruccion);
    if (strcmp(instruccion, "MOV_IN") == 0)
        return MOV_IN;
    else if (strcmp(instruccion, "MOV_OUT") == 0)
        return MOV_OUT;
    else if (strcmp(instruccion, "I/O") == 0)
        return IO;
    else if (strcmp(instruccion, "F_OPEN") == 0)
        return F_OPEN;
    else if (strcmp(instruccion, "F_CLOSE") == 0)
        return F_CLOSE;
    else if (strcmp(instruccion, "F_SEEK") == 0)
        return F_SEEK;
    else if (strcmp(instruccion, "F_READ") == 0)
        return F_READ;
    else if (strcmp(instruccion, "F_WRITE") == 0)
        return F_WRITE;
    else if (strcmp(instruccion, "F_TRUNCATE") == 0)
        return F_TRUNCATE;
    else if (strcmp(instruccion, "WAIT") == 0)
        return WAIT;
    else if (strcmp(instruccion, "SIGNAL") == 0)
        return SIGNAL;
    else if (strcmp(instruccion, "CREATE_SEGMENT") == 0)
        return CREATE_SEGMENT;
    else if (strcmp(instruccion, "DELETE_SEGMENT") == 0)
        return DELETE_SEGMENT;
    else if (strcmp(instruccion, "YIELD") == 0)
        return YIELD;
    else if (strcmp(instruccion, "EXIT") == 0)
        return EXIT;

    return EXIT;
}

char *descripcion_estado(ESTADO estado)
{
    switch (estado)
    {
    case NEW:
        return "NEW";
    case READY:
        return "READY";
    case EXEC:
        return "EXEC";
    case BLOCK:
        return "BLOCK";
    case FINISHED:
        return "FINISHED";
    }
}

void cambiar_estado(Proceso *proceso, ESTADO estado)
{
    ESTADO anterior = proceso->estado;
    proceso->estado = estado;
    log_warning(logger, "[KERNEL] Proceso PID:<%d> - Estado Anterior: <%s> - Estado Actual: <%s>", proceso->pcb->PID, descripcion_estado(anterior), descripcion_estado(proceso->estado));
}

void quitar_salto_de_linea(char *cadena)
{
    int longitud = strcspn(cadena, "\n");
    cadena[longitud] = '\0';
}