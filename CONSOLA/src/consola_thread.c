#include "consola_thread.h"
#include "protocolo.h"

void enviar_instrucciones_a_kernel(t_list* instrucciones_archivo)
{

    log_info(logger, "[CONSOLA]: Creando paquete de instrucciones");

      Instruccion2* instrucciones1 = (Instruccion2*)malloc(sizeof(Instruccion2));

        instrucciones1->nombreInstruccion = "SET AX";
        instrucciones1->valor = 42;
        instrucciones1->valorChar = "HOLA";
        instrucciones1->registro = "AX";
        instrucciones1->direccionLogica = 123;
        instrucciones1->tiempo = 456;
        instrucciones1->nombreArchivo = "archivo.txt";
        instrucciones1->posicion = 789;
        instrucciones1->cantBytes = 10;
        instrucciones1->recurso = "memoria";
        instrucciones1->idSegmento = 999;

        Instruccion2* instrucciones2 = (Instruccion2*)malloc(sizeof(Instruccion2));

        instrucciones2->nombreInstruccion = "SET2 AX2";
        instrucciones2->valor = 42;
        instrucciones2->valorChar = "";
        instrucciones2->registro = "";
        instrucciones2->direccionLogica = 123;
        instrucciones2->tiempo = 456;
        instrucciones2->nombreArchivo = "archivo.txt";
        instrucciones2->posicion = 789;
        instrucciones2->cantBytes = 10;
        instrucciones2->recurso = "memoria";
        instrucciones2->idSegmento = 999;

    t_list* instrucciones = list_create();
    list_add(instrucciones, instrucciones1);
    list_add(instrucciones, instrucciones2);
        
    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL: preparando paquete");

    PAQUETE *paquete_instrucciones = crear_paquete(INSTRUCCIONES);

    log_info(logger, "[CONSOLA]: Enviar INSTRUCCIONES a KERNEL:: serializar instrucciones ");

    // for(int i = 0; i < list_size(instrucciones_archivo); i++){
    //     Instruccion2* instruccion = list_get(instrucciones_archivo, i);
    //     log_info(logger, "Instruccion %d: nombre: %s - registro: %s", i, instruccion->nombreInstruccion, instruccion->registro);
    // }

    paquete_instrucciones->buffer = serializar_instrucciones(instrucciones_archivo);

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
