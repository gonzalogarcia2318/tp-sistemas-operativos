#include "kernel_config.h"

KERNEL_CONFIG KernelConfig;

void rellenar_configuracion_kernel(Config *config)
{
    KernelConfig.IP = config_get_string_value(config, "IP");
    KernelConfig.PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
    KernelConfig.IP_CPU = config_get_string_value(config, "IP_CPU");
    KernelConfig.PUERTO_CPU = config_get_string_value(config, "PUERTO_CPU");
    KernelConfig.IP_MEMORIA = config_get_string_value(config, "IP_MEMORIA");
    KernelConfig.PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
    KernelConfig.IP_FILESYSTEM = config_get_string_value(config, "IP_FILESYSTEM");
    KernelConfig.PUERTO_FILESYSTEM = config_get_string_value(config, "PUERTO_FILESYSTEM");
    KernelConfig.ALGORITMO_PLANIFICACION = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    KernelConfig.ESTIMACION_INICIAL = config_get_string_value(config,"ESTIMACION_INICIAL");
    KernelConfig.HRRN_ALFA = config_get_string_value(config, "HRRN_ALFA");
    KernelConfig.GRADO_MAX_MULTIPROGRAMACION = config_get_string_value(config, "GRADO_MAX_MULTIPROGRAMACION");
    KernelConfig.RECURSOS = config_get_array_value(config, "RECURSOS");
    KernelConfig.INSTANCIAS_RECURSOS = config_get_array_value(config, "INSTANCIAS_RECURSOS");
}