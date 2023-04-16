#include <consola_config.h>

CONSOLA_CONFIG ConsolaConfig;

void rellenar_configuracion_consola(Config *config)
{
  ConsolaConfig.IP_KERNEL = config_get_string_value(config, "IP_KERNEL");
  ConsolaConfig.PUERTO_KERNEL = config_get_int_value(config, "PUERTO_KERNEL");
}