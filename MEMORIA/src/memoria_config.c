#include <memoria_config.h>

MEMORIA_CONFIG MemoriaConfig;

void rellenar_configuracion_memoria(Config *config)
{
    MemoriaConfig.IP = config_get_string_value(config, "IP");
    MemoriaConfig.PUERTO_ESCUCHA = config_get_int_value(config, "PUERTO_ESCUCHA");
    MemoriaConfig.TAM_MEMORIA = config_get_int_value(config, "TAM_MEMORIA");
    MemoriaConfig.TAM_SEGMENTO_0 = config_get_int_value(config, "TAM_SEGMENTO_0");
    MemoriaConfig.CANT_SEGMENTOS = config_get_int_value(config, "CANT_SEGMENTOS");
    MemoriaConfig.RETARDO_MEMORIA = config_get_int_value(config, "RETARDO_MEMORIA");
    MemoriaConfig.RETARDO_COMPACTACION = config_get_int_value(config, "RETARDO_COMPACTACION");
    MemoriaConfig.ALGORITMO_ASIGNACION = config_get_string_value(config, "ALGORITMO_ASIGNACION");
}