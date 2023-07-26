#ifndef CONSOLA_UTILS_H
#define CONSOLA_UTILS_H

#define SUCCESS 0
#define FAILURE -1

#define ARCHIVO_LOGGER "config/consola.log"
// #define ARCHIVO_CONFIG "config/consola.config"
// #define ARCHIVO_CODIGO "pseudocodigo.txt"

#include "protocolo.h"
#include "consola_config.h"
#include "server.h"
#include "cliente.h"
#include "consola_thread.h"

extern int socket_kernel;
extern Logger *logger;
extern Config *config;

void test1();

void inicializar_logger_consola();
void inicializar_config_consola();
int conectar_con_kernel(void);
int desconectar_con_kernel(void);
void terminar_consola(void);
void liberar_instruccion(Instruccion *instruccion);

Instruccion *parsear_instruccion_por_linea(char *linea);
t_list *leer_instrucciones(char *path_instrucciones);

#endif