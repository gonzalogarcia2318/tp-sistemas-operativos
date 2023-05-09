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
void terminar_ejecucion();
void conectar_con_kernel();
int conectar_con_memoria();

//Cambiar de void a estructura de PCB //todo
//[SET] seria un struct con  la instruccion y los valores que necesite
void recibir_instrucciones(); //RECIBE PCB, ES GENERAL PARA TODAS, INCLUYE FETCH
void decode_instruccion();//RECIBE INSTRUCCION
void ejecutar_instruccion(); //EXECUTE

#endif