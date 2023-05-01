#include "file_system_config.h"

FILE_SYSTEM_CONFIG FileSystemConfig;

void rellenar_configuracion_file_system(Config *config)
{
	FileSystemConfig.IP = config_get_string_value(config, "IP");
	FileSystemConfig.PUERTO_ESCUCHA = config_get_string_value(config, "PUERTO_ESCUCHA");
	FileSystemConfig.PUERTO_MEMORIA = config_get_string_value(config, "PUERTO_MEMORIA");
	FileSystemConfig.PATH_SUPERBLOQUE = config_get_string_value(config, "PATH_SUPERBLOQUE");
	FileSystemConfig.PATH_BITMAP = config_get_string_value(config, "PATH_BITMAP");
	FileSystemConfig.PATH_BLOQUES = config_get_string_value(config, "PATH_BLOQUES");
	FileSystemConfig.PATH_FCB = config_get_string_value(config, "PATH_FCB");
	FileSystemConfig.RETARDO_ACCESO_BLOQUE = config_get_string_value(config, "RETARDO_ACCESO_BLOQUE");
}
