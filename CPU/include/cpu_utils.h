#ifndef CPU_UTILS_H
#define CPU_UTILS_H

#include "protocolo.h"
#include "cpu_config.h"
#include "cpu_thread.h"

#define ARCHIVO_LOGGER "config/cpu.log"
#define ARCHIVO_CONFIG "config/cpu.config"

#define SUCCESS 0
#define FAILURE -1


extern Logger *logger;
extern Config *config;
extern Hilo hilo_kernel;
extern int socket_cpu;
extern int socket_memoria;
extern int socket_cliente;
extern int socket_kernel;


void iniciar_logger_cpu();
void iniciar_config_cpu();
int iniciar_servidor_cpu();
void conectar_con_kernel();
int conectar_con_memoria();
void terminar_ejecucion();


void manejar_instrucciones(PCB*); //ES GENERAL PARA TODAS, INCLUYE FETCH

    int decode_instruccion();
        bool esSet(Instruccion*);
        void aplicar_retardo(int32_t);
        bool requiere_traduccion(Instruccion*);
        int32_t realizar_traduccion(int32_t, t_list*);
            int obtener_num_segmento(int32_t);
            int obtener_desplazamiento_segmento(int32_t);
        bool comprobar_segmentation_fault(int32_t, Instruccion*, t_list*);
        void avisar_seg_fault_kernel(PCB*, Instruccion*);
    
    int ejecutar_instruccion();
        void asignar_a_registro (char*, char*, PCB*);
        char* obtener_valor_registro(Registro_CPU,char*);
        
        void ejecutar_set(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_mov_in(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_mov_out(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_IO(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_f_open(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_f_close(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_f_seek(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_f_read(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_f_write(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_f_truncate(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_wait(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_signal(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_create_segment(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_delete_segment(PAQUETE*,Instruccion*,PCB*);
        void ejecutar_yield(PAQUETE*,PCB*);
        void ejecutar_exit(PAQUETE*,PCB*);

#endif