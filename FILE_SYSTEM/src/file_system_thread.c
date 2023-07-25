#include "file_system_thread.h"

extern t_bitarray *bitmap;
extern SUPERBLOQUE superbloque;
extern FILE *archivo_bloques;

bool manejar_paquete_kernel(int socket_kernel)
{

    while (true)
    {
        char *mensaje;
        switch (obtener_codigo_operacion(socket_kernel))
        {
        case MENSAJE:
            mensaje = obtener_mensaje_del_cliente(socket_kernel);
            log_info(logger, "[FILE_SYSTEM]: Mensaje recibido de KERNEL: %s", mensaje);
            free(mensaje);
            break;

        case INSTRUCCION:
            log_info(logger, "[FILE SYSTEM]: INSTRUCCION recibida de KERNEL");
            recibir_instruccion_kernel();
            log_info(logger, "[FILE_SYSTEM]: Esperando conexiones de Kernel...");
            break;

        case DESCONEXION:
            log_warning(logger, "[FILE_SYSTEM]: Conexión de KERNEL terminada.");
            return true;

        default:
            log_warning(logger, "[FILE_SYSTEM]: Operacion desconocida desde KERNEL.");
            break;
        }
    }
}

void recibir_instruccion_kernel()
{

    BUFFER *buffer = recibir_buffer(socket_kernel);
    int32_t cod_instruccion;
    char *nombre_archivo;
    uint32_t tamanio_nombre;

    memcpy(&cod_instruccion, buffer->stream + sizeof(int32_t), sizeof(int32_t));
    buffer->stream += (sizeof(int32_t) * 2); // *2 por tamaño y valor

    memcpy(&tamanio_nombre, buffer->stream, sizeof(int32_t));
    buffer->stream += (sizeof(int32_t));

    log_info(logger, "tamaño nombre: %d", tamanio_nombre);

    nombre_archivo = malloc(tamanio_nombre);

    memcpy(nombre_archivo, buffer->stream, tamanio_nombre);
    buffer->stream += tamanio_nombre;

    int32_t direccion_fisica = 0;
    int32_t tamanio = 0;
    int32_t puntero_archivo = 0;
    int32_t pid = 0;

    switch (cod_instruccion)
    {
    case CREAR_ARCHIVO:
        log_warning(logger, "CREAR ARCHIVO: <NOMBRE_ARCHIVO: %s>", nombre_archivo);
        if (crear_archivo(nombre_archivo) != -1)
        {
            log_info(logger, "FCB CREADO DE: %s>", nombre_archivo);

            enviar_respuesta_kernel(SUCCESS, RESPUESTA_FILE_SYSTEM);
        }
        break;
    case EXISTE_ARCHIVO:
        log_warning(logger, "ABRIR ARCHIVO: <NOMBRE_ARCHIVO: %s>", nombre_archivo);
        if (existe_archivo(nombre_archivo) == SUCCESS)
        {

            enviar_respuesta_kernel(SUCCESS, RESPUESTA_FILE_SYSTEM);
        }
        else
        {
            enviar_respuesta_kernel(FAILURE, RESPUESTA_FILE_SYSTEM);
        }
        break;

    case F_READ:

        memcpy(&puntero_archivo, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
        memcpy(&tamanio, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
        memcpy(&direccion_fisica, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
        memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);

        log_warning(logger, "LEER ARCHIVO: <NOMBRE_ARCHIVO: %s> - <PUNTERO ARCHIVO: %d> - <DIRECCION MEMORIA: %d> - <TAMAÑO: %d>",
                    nombre_archivo,
                    puntero_archivo,
                    direccion_fisica,
                    tamanio);

        if (ejecutar_f_read(nombre_archivo, puntero_archivo, tamanio, direccion_fisica, pid) == SUCCESS)
        {
            enviar_respuesta_kernel(SUCCESS, FINALIZO_LECTURA);
        }
        else
        {
            enviar_respuesta_kernel(FAILURE, FINALIZO_LECTURA);
        }

        break;

    case F_WRITE:
        memcpy(&puntero_archivo, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
        memcpy(&tamanio, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
        memcpy(&direccion_fisica, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);
        memcpy(&pid, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);

        log_warning(logger, "ESCRIBIR ARCHIVO: <NOMBRE_ARCHIVO: %s> - <PUNTERO ARCHIVO: %d> - <DIRECCION MEMORIA: %d> - <TAMAÑO: %d>",
                    nombre_archivo,
                    puntero_archivo,
                    direccion_fisica,
                    tamanio);

        if (ejecutar_f_write(nombre_archivo, puntero_archivo, direccion_fisica, tamanio, pid) == SUCCESS)
        { // TODO
            enviar_respuesta_kernel(SUCCESS, FINALIZO_ESCRITURA);
        }
        else
        {
            enviar_respuesta_kernel(FAILURE, FINALIZO_ESCRITURA);
        }
        break;

    case F_TRUNCATE:
        memcpy(&tamanio, buffer->stream + sizeof(int32_t), sizeof(int32_t));
        buffer->stream += (sizeof(int32_t) * 2);

        log_warning(logger, "TRUNCAR ARCHIVO: <NOMBRE_ARCHIVO: %s> - Tamaño: <TAMAÑO: %d>",
                    nombre_archivo,
                    tamanio);

        ejecutar_f_truncate(nombre_archivo, tamanio);

        enviar_respuesta_kernel(SUCCESS, FINALIZO_TRUNCADO);
        break;

    default:
        log_error(logger, "FILE SYSTEM: ERROR: COD_INSTRUCCION DESCONOCIDO");
        enviar_respuesta_kernel(FAILURE, RESPUESTA_FILE_SYSTEM);
        break;
    }
}

// crea un archivo FCB correspondiente al nuevo archivo, con tamaño 0 y sin bloques asociados.
int crear_archivo(char *nombre)
{
    // mejorar
    // Config * config = config_create("config/file_system.config");
    // char* pathCompleto = config_get_string_value(config, "PATH_FCB");
    log_info(logger, "CREAR ARCHIVO - %s", nombre);

    char *pathFcb = FileSystemConfig.PATH_FCB;

    char *pathCompleto = string_duplicate(pathFcb);

    // Crea el directorio
    mkdir(pathCompleto, 0777);

    string_append(&pathCompleto, nombre);
    string_append(&pathCompleto, ".config");

    t_config fcb_config;
    fcb_config.path = pathCompleto;

    t_dictionary *diccionario = dictionary_create();
    dictionary_put(diccionario, "NOMBRE_ARCHIVO", nombre);
    dictionary_put(diccionario, "TAMANIO_ARCHIVO", "0");
    dictionary_put(diccionario, "PUNTERO_DIRECTO", "0");
    dictionary_put(diccionario, "PUNTERO_INDIRECTO", "0");
    fcb_config.properties = diccionario;

    // log_info(logger, "Crear archivo %s - en path: %s", nombre, pathCompleto);

    int res = config_save_in_file(&fcb_config, pathCompleto);

    free(pathCompleto);

    return res;
}

int existe_archivo(char *nombre)
{
    // mejorar
    //  Config * config = config_create("config/file_system.config");
    //  char* pathCompleto = config_get_string_value(config, "PATH_FCB");
    // log_info(logger, "EXISTE ARCHIVO - %s", nombre);
    char *pathFcb = FileSystemConfig.PATH_FCB;
    // char *pathFcb = "config/fcb/";
    // log_info(logger, "pathFcb: %s", pathFcb);

    char *pathCompleto = string_duplicate(pathFcb);

    string_append(&pathCompleto, nombre);
    string_append(&pathCompleto, ".config");

    // log_info(logger, "pathCompleto: %s", pathCompleto);

    t_config *fcb = config_create(pathCompleto);

    free(pathCompleto);

    if (fcb == NULL)
    {
        log_info(logger, "FCB == NULL => No existe archivo");
        return FAILURE;
    }
    else
    {
        log_info(logger, "FCB != NULL =>  existe archivo");
        return SUCCESS;
    }
}

void ejecutar_f_truncate(char *nombre, int a_truncar)
{

    char *pathFcb = FileSystemConfig.PATH_FCB;
    char *pathCompleto = string_duplicate(pathFcb);

    string_append(&pathCompleto, nombre);
    string_append(&pathCompleto, ".config");
    // string_append(&path,nombre);
    // string_append(&path,".config");
    t_config *fcb = config_create(pathCompleto);
    int tamanio = config_get_int_value(fcb, "TAMANIO_ARCHIVO");
    uint32_t puntero_directo = config_get_int_value(fcb, "PUNTERO_DIRECTO");
    uint32_t puntero_indirecto = config_get_int_value(fcb, "PUNTERO_INDIRECTO");
    // buffers
    char puntero_directo_str[MAX_CARACTERES_PUNTERO];
    char puntero_indirecto_str[MAX_CARACTERES_PUNTERO];
    char tamanio_archivo_str[MAX_CARACTERES_PUNTERO];

    if (a_truncar > tamanio)
    {
        // calcular bloques necesarios
        int bloques_necesarios = ceil((double)(a_truncar - tamanio) / superbloque.BLOCK_SIZE);
        // int bloques_minimos = (bloques_necesarios >=2) ? 2 : 1;

        // buscar bloques libres
        if (tamanio == 0)
        {
            puntero_directo = buscar_bloque_libre();
            // Convertir uint32_t a string
            log_info(logger, "puntero directo: %d", puntero_directo);
            size_t longitud = strlen(puntero_directo_str);
            log_info(logger, "tamanio  buffer: %%zu\n", longitud);
            snprintf(puntero_directo_str, sizeof(puntero_directo_str), "%u", puntero_directo);
            config_set_value(fcb, "PUNTERO_DIRECTO", puntero_directo_str);
            bloques_necesarios--;
        }
        if (bloques_necesarios >= 1)
        {
            if (puntero_indirecto == 0)
                puntero_indirecto = buscar_bloque_libre();

            archivo_bloques = fopen(FileSystemConfig.PATH_BLOQUES, "wb+");

            for (int bloques_restantes = bloques_necesarios - 1; bloques_restantes >= 0; bloques_restantes--)
            {
                int pos_dentro_bloque = (bloques_necesarios - bloques_restantes - 1) * 4; // es 4 Porque eso ocupa un Uint32t
                fseek(archivo_bloques, puntero_indirecto + pos_dentro_bloque, SEEK_SET);

                uint32_t bloque_sgt = buscar_bloque_libre();
                aplicar_retardo_acceso_bloque();
                log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                            nombre,
                            bloques_restantes,
                            bloque_sgt / superbloque.BLOCK_SIZE);

                fwrite(&bloque_sgt, sizeof(uint32_t), 1, archivo_bloques);
            }
            fclose(archivo_bloques);
            snprintf(puntero_indirecto_str, sizeof(puntero_indirecto_str), "%u", puntero_indirecto);
            config_set_value(fcb, "PUNTERO_INDIRECTO", puntero_indirecto_str);
        }
        // actualizar el tamaño del archivo en FCB
        snprintf(tamanio_archivo_str, sizeof(tamanio_archivo_str), "%u", a_truncar);
        config_set_value(fcb, "TAMANIO_ARCHIVO", tamanio_archivo_str);
    }
    else if (a_truncar < tamanio) // reducir tamanio
    {
        int a_reducir = tamanio - a_truncar;
        int cant_ptrs = (tamanio / superbloque.BLOCK_SIZE) - 1; // se restan  por el ptr directo
        int bloques_restantes = ceil((double)(a_reducir) / superbloque.BLOCK_SIZE);
        char *pathBitmap = FileSystemConfig.PATH_BITMAP;
        char *path = string_duplicate(pathBitmap);
        FILE *bm = fopen(path, "rb+");
        // fread(bitmap, sizeof(bitmap->size), 1, bm);

        if (bloques_restantes > 1)
        {
            archivo_bloques = fopen(FileSystemConfig.PATH_BLOQUES, "rb+");
            int copybr = bloques_restantes;
            for (int i = 1; i <= bloques_restantes; i++)
            {
                uint32_t valor_puntero;
                uint32_t pos_puntero = puntero_indirecto + (sizeof(uint32_t) * copybr);

                fseek(archivo_bloques, pos_puntero, SEEK_SET);
                fread(&valor_puntero, sizeof(uint32_t), 1, archivo_bloques);
                // marcar como libres en el bitmap
                bitarray_clean_bit(bitmap, valor_puntero / superbloque.BLOCK_SIZE);
                log_warning(logger, "ACCESO A BITMAP: <NUMERO BLOQUE: %ld> - Estado: <ESTADO>: %d>",
                            valor_puntero / superbloque.BLOCK_SIZE,
                            0);
                copybr--;
                // se dejan los valores en el archivo de bloques, pero al marcarse libres, otro proceso pisara los valores
            }
        }
        else
        {
            archivo_bloques = fopen(FileSystemConfig.PATH_BLOQUES, "rb+");
            for (int j = 1; j <= cant_ptrs; j++)
            {
                uint32_t valor_puntero;
                uint32_t pos_puntero = puntero_indirecto + (sizeof(uint32_t) * cant_ptrs);

                fseek(archivo_bloques, pos_puntero, SEEK_SET);
                fread(&valor_puntero, sizeof(uint32_t), 1, archivo_bloques);
                aplicar_retardo_acceso_bloque();
                log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                            nombre,
                            puntero_indirecto / superbloque.BLOCK_SIZE,
                            valor_puntero / superbloque.BLOCK_SIZE);
                // marcar como libres en el bitmap
                bitarray_clean_bit(bitmap, valor_puntero / superbloque.BLOCK_SIZE);
                log_warning(logger, "ACCESO A BITMAP: <NUMERO BLOQUE: %ld> - Estado: <ESTADO>: %d>",
                            valor_puntero / superbloque.BLOCK_SIZE,
                            0);
                // se dejan los valores en el archivo de bloques, pero al marcarse libres, otro proceso pisara los valores
            }
        }
        fwrite(bitmap->bitarray, bitmap->size, 1, bm);
        fclose(bm);
        fclose(archivo_bloques); // cerrar para el reducir
        free(path);
        // actualizar el tamaño del archivo en FCB
        snprintf(tamanio_archivo_str, sizeof(tamanio_archivo_str), "%u", a_truncar);
        config_set_value(fcb, "TAMANIO_ARCHIVO", tamanio_archivo_str);
    }
    free(pathCompleto);
    config_save(fcb);
}

int buscar_bloque_libre()
{
    int ptr = 0;
    off_t index;
    char *pathBitmap = FileSystemConfig.PATH_BITMAP;
    char *path = string_duplicate(pathBitmap);
    FILE *file = fopen(path, "rb+");

    for (index = 0; index < bitmap->size; index++)
    {
        bool estado = bitarray_test_bit(bitmap, index);
        log_warning(logger, "ACCESO A BITMAP: <NUMERO BLOQUE: %ld> - Estado: <ESTADO>: %d>",
                    index,
                    estado);
        if (!estado)
        {
            break;
        }
    }
    ptr = index * superbloque.BLOCK_SIZE;

    bitarray_set_bit(bitmap, index);
    fwrite(bitmap->bitarray, bitmap->size, 1, file);
    fclose(file);
    free(path);
    return ptr;
}

int ejecutar_f_read(char *nombre_archivo, uint32_t puntero_archivo, int tamanio, int direccion_fisica, int32_t pid)
{
    // abrir archivo de bloques
    archivo_bloques = fopen(FileSystemConfig.PATH_BLOQUES, "rb+");
    char *valor_leido = malloc(tamanio);

    fseek(archivo_bloques, puntero_archivo, SEEK_SET);
    fread(valor_leido, tamanio, 1, archivo_bloques);
    valor_leido[tamanio] = '\0';

    log_info(logger, "NOMBRE_ARCHIVO>: %s - VALOR LEIDO: %s", nombre_archivo, valor_leido);

    log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                nombre_archivo,
                puntero_archivo / superbloque.BLOCK_SIZE, //cambiar por el del archivo
                puntero_archivo / superbloque.BLOCK_SIZE);

    aplicar_retardo_acceso_bloque();

    fclose(archivo_bloques);

    int estado = enviar_a_memoria(direccion_fisica, valor_leido, pid);

    return estado;
}

int ejecutar_f_write(char *nombre_archivo, uint32_t puntero_archivo, uint32_t direccion_fisica, int32_t tamanio, int32_t pid)
{

    archivo_bloques = fopen(FileSystemConfig.PATH_BLOQUES, "wb+");

    char *datos = obtener_info_de_memoria(direccion_fisica, tamanio, pid);

    log_info(logger, "DATO: %s", datos);

    fseek(archivo_bloques, puntero_archivo, SEEK_SET);
    fwrite(datos, tamanio, 1, archivo_bloques);

    log_warning(logger, "ACCESO A BlOQUE: Archivo: <NOMBRE_ARCHIVO>: %s - Bloque Archivo: <NUMERO BLOQUE ARCHIVO>:%d - Bloque File System <NUMERO BLOQUE FS>: %d",
                nombre_archivo,
                puntero_archivo / superbloque.BLOCK_SIZE, //cambiar por el del archivo
                puntero_archivo / superbloque.BLOCK_SIZE);

    aplicar_retardo_acceso_bloque();

    log_info(logger, "VALOR ESCRITO: Archivo: <DATOS>: %s",
             datos);

    // QUITAR
    // fseek(archivo_bloques,puntero_archivo,SEEK_SET);
    // char *test = malloc(16);
    // fread(test,16,1,archivo_bloques);
    // log_warning(logger, "VALOR LeiDO: Archivo: <DATOS>: %s",test);
    // QUITAR

    fclose(archivo_bloques);
    return SUCCESS;
}

int enviar_a_memoria(int32_t direccion, char *valor, int32_t pid)
{
    PAQUETE *paquete = crear_paquete(WRITE);

    int tamanio = strlen(valor);
    agregar_a_paquete(paquete, &direccion, sizeof(int32_t));
    agregar_a_paquete(paquete, &tamanio, sizeof(int32_t));
    agregar_a_paquete(paquete, valor, tamanio * sizeof(char));
    agregar_a_paquete(paquete, &pid, sizeof(int32_t));
    enviar_paquete_a_servidor(paquete, socket_memoria);
    eliminar_paquete(paquete);

    BUFFER *buffer;

    switch (obtener_codigo_operacion(socket_memoria))
    {
    case WRITE:
        log_info(logger, "[FILE_SYSTEM]: MEMORIA ESCRIBIO CORRECTAMENTE");

        buffer = recibir_buffer(socket_memoria);
        int respuesta;
        memcpy(&respuesta, buffer->stream + sizeof(int32_t), sizeof(int32_t));

        return SUCCESS;
        break;
    default:
        log_error(logger, "[FILE_SYSTEM]: NO SE PUDO ESCRIBIR EN MEMORIA");
        return FAILURE;
        break;
    }
}

void enviar_respuesta_kernel(int ok, CODIGO_OPERACION cod)
{
    log_info(logger, "Se va a enviar a kernel un: %d",
             ok);

    PAQUETE *paquete_kernel = crear_paquete(cod);
    agregar_a_paquete(paquete_kernel, &ok, sizeof(int32_t));
    enviar_paquete_a_cliente(paquete_kernel, socket_kernel);
    eliminar_paquete(paquete_kernel);
    log_info(logger, "Se envio correctamente a kernel");
}

char *obtener_info_de_memoria(int32_t dir_fisica, uint32_t tamanio, int32_t pid)
{
    PAQUETE *paquete = crear_paquete(READ);

    agregar_a_paquete(paquete, &dir_fisica, sizeof(int32_t));
    agregar_a_paquete(paquete, &tamanio, sizeof(uint32_t));
    agregar_a_paquete(paquete, &pid, sizeof(uint32_t));
    enviar_paquete_a_servidor(paquete, socket_memoria);
    eliminar_paquete(paquete);

    switch (obtener_codigo_operacion(socket_memoria))
    {
    case READ:
        log_info(logger, "[FILE_SYSTEM]: MEMORIA LEYO CORRECTAMENTE");
        BUFFER *buffer = recibir_buffer(socket_memoria);

        int32_t tamanio_dato;
        memcpy(&tamanio_dato, buffer->stream, sizeof(int32_t));
        buffer->stream += sizeof(int32_t);

        // log_info(logger, "tamanio_dato; %d", tamanio_dato);

        char *dato = malloc(tamanio_dato);

        memcpy(dato, buffer->stream, tamanio_dato);
        buffer->stream += tamanio_dato;

        dato[tamanio_dato] = '\0';

        return dato;
    default:
        log_error(logger, "[FILE_SYSTEM]: MEMORIA FALLO AL LEER");
        return;
        break;
    }
}
