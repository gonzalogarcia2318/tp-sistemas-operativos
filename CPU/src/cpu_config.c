#include "cpu_config.h"

CPU_CONFIG CPUConfig;

void rellenar_configuracion_cpu(Config *config)
{
    CPUConfig.IP = config_get_string_value(config, "IP");
    CPUConfig.RETARDO_INSTRUCCION = config_get_int_value(config, "RETARDO_INSTRUCCION");
    CPUConfig.IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
    CPUConfig.PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
    CPUConfig.PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
    CPUConfig.TAM_MAX_SEGMENTO = config_get_int_value(config, "TAM_MAX_SEGMENTO");
}