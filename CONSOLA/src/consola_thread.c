#include "consola_thread.h"
#include "protocolo.h"

void enviar_instrucciones_a_kernel()
{

    log_info(logger, "[CONSOLA]: Creando paquete de instrucciones");

      Instruccion2* instrucciones = (Instruccion2*)malloc(sizeof(Instruccion2));

        instrucciones->nombreInstruccion = "SET AX";
        instrucciones->nombreInstruccion_long = 6;
        instrucciones->valor = 42;
        instrucciones->valorChar = "HOLA";
        instrucciones->valorChar_long = 4;
        instrucciones->registro = "AX";
        instrucciones->registro_long=2;
        instrucciones->direccionLogica = 123;
        instrucciones->tiempo = 456;
        instrucciones->nombreArchivo = "archivo.txt";
        instrucciones->nombreARchivoLength = strlen(instrucciones->nombreArchivo);
        instrucciones->posicion = 789;
        instrucciones->cantBytes = 10;
        instrucciones->recurso = "memoria";
        instrucciones->recurso_long = 7;
        instrucciones->idSegmento = 999;
        
    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL: preparando paquete");

    PAQUETE *paquete_instrucciones = crear_paquete(INSTRUCCIONES);

    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL:: serializar instrucciones ");

    paquete_instrucciones->buffer = serializar_instrucciones(instrucciones);

    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL: enviar");

    enviar_paquete_a_servidor(paquete_instrucciones, socket_kernel);

}

void escuchar_kernel()
{
    
    while (1)
    {
        switch (obtener_codigo_operacion(socket_kernel))
        {
        case DESCONEXION:
            log_warning(logger, "[CONSOLA]: Kernel se desconecto.");
            return;

        default:
            continue;
        }

        break;
    }
    
}
