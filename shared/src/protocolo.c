#include "protocolo.h"

PAQUETE *crear_paquete(CODIGO_OPERACION codigoOperacion)
{
  PAQUETE *paquete = malloc(sizeof(PAQUETE));

  paquete->codigo_operacion = codigoOperacion;
  inicializar_buffer(paquete);

  return paquete;
}

void eliminar_paquete(PAQUETE *paquete)
{
  if (paquete != NULL)
  {
    if (paquete->buffer != NULL)
    {
      free(paquete->buffer->stream);
      free(paquete->buffer);
    }
    free(paquete);
  }
}

void inicializar_buffer(PAQUETE *paquete)
{
  paquete->buffer = malloc(sizeof(BUFFER));
  paquete->buffer->size = 0;
  paquete->buffer->stream = NULL;
}

void agregar_a_paquete(PAQUETE *paquete, void *valor, int tamanio)
{
  paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

  memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
  memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

  paquete->buffer->size += tamanio + sizeof(int);
}

void *serializar_paquete(PAQUETE *paquete, int bytes)
{
  void *magic = malloc(bytes);
  int desplazamiento = 0;

  memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
  desplazamiento += sizeof(int);
  memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
  desplazamiento += sizeof(int);
  memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);

  return magic;
}

void enviar_paquete_a_cliente(PAQUETE *paquete, int socketCliente)
{
  enviar_paquete_a_servidor(paquete, socketCliente);
}

void enviar_mensaje_a_cliente(char *mensaje, int socketCliente)
{
  PAQUETE *paquete = crear_paquete(MENSAJE);
  agregar_a_paquete(paquete, mensaje, strlen(mensaje) + 1);
  enviar_paquete_a_cliente(paquete, socketCliente);
  eliminar_paquete(paquete);
}

void enviar_paquete_a_servidor(PAQUETE *paquete, int socketCliente)
{
  int bytes = paquete->buffer->size + 2 * sizeof(int);
  void *aEnviar = serializar_paquete(paquete, bytes);

  send(socketCliente, aEnviar, bytes, 0);
  free(aEnviar);
}

void enviar_mensaje_a_servidor(char *mensaje, int socketCliente)
{
  PAQUETE *paquete = crear_paquete(MENSAJE);

  paquete->buffer->size = strlen(mensaje) + 1;
  paquete->buffer->stream = malloc(paquete->buffer->size);
  memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

  enviar_paquete_a_servidor(paquete, socketCliente);
  eliminar_paquete(paquete);
}

CODIGO_OPERACION obtener_codigo_operacion(int socketCliente)
{
  CODIGO_OPERACION codigoOperacion;

  if (recv(socketCliente, &codigoOperacion, sizeof(int), MSG_WAITALL) > 0)
    return codigoOperacion;
  else
  {
    close(socketCliente);
    return DESCONEXION;
  }
}

void *obtener_buffer_del_cliente(int *tamanio, int socketCliente)
{
  void *buffer;

  recv(socketCliente, tamanio, sizeof(int), MSG_WAITALL);
  buffer = malloc(*tamanio);
  recv(socketCliente, buffer, *tamanio, MSG_WAITALL);


  return buffer;
}

BUFFER* recibir_buffer(int socket) {
	BUFFER* buffer = malloc(sizeof(BUFFER));

	recv(socket, &(buffer->size), sizeof(int), 0);
	buffer->stream = malloc(buffer->size);
	recv(socket, buffer->stream, buffer->size, 0);

	return buffer;
}

void * obtener_paquete_estructura_dinamica(int socketCliente){

  int tamanioBuffer;

  void *buffer = obtener_buffer_del_cliente(&tamanioBuffer, socketCliente);

  Instruccion2* instrucciones = deserializar_instrucciones(buffer);

  return instrucciones;
  
}

char *obtener_mensaje_del_cliente(int socketCliente)
{
  int tamanio;
  char *mensaje = obtener_buffer_del_cliente(&tamanio, socketCliente);

  return mensaje;
}

Lista *obtener_paquete_como_lista(int socketCliente)
{
  int tamanioBuffer;
  int tamanioContenido;
  int desplazamiento = 0;

  Lista *contenido = list_create();
  void *buffer = obtener_buffer_del_cliente(&tamanioBuffer, socketCliente);

  while (desplazamiento < tamanioBuffer)
  {
    memcpy(&tamanioContenido, buffer + desplazamiento, sizeof(int));
    desplazamiento += sizeof(int);

    void *valor = malloc(tamanioContenido);
    memcpy(valor, buffer + desplazamiento, tamanioContenido);
    desplazamiento += tamanioContenido;

    list_add(contenido, valor);
  }

  free(buffer);
  return contenido;
}

char *obtener_mensaje_del_servidor(int socketServidor)
{
  Lista *listaMensaje;
  char *mensaje;

  switch (obtener_codigo_operacion(socketServidor))
  {
  case MENSAJE:
    listaMensaje = obtener_paquete_como_lista(socketServidor);
    mensaje = ((char *)list_get(listaMensaje, 0));
    list_destroy_and_destroy_elements(listaMensaje, &free);
    break;

  default:
    break;
  }

  return mensaje;
}



// TODO: MOVER A OTRO ARCHIVO


BUFFER *serializar_pcb(PCB *pcb)
{
    printf("Serializar pcb \n");
    printf("pcb id %d \n", pcb->PID);
    printf("pcb pc %d \n", pcb->program_counter);

    BUFFER *buffer = malloc(sizeof(PCB));

    buffer->size = sizeof(int32_t) * 2;

    void *stream = malloc(buffer->size);
    int offset = 0;

    printf("1 \n");
    printf("strem %s \n", stream + offset );

    memcpy(stream + offset, &pcb->PID, sizeof(int32_t));
    printf("2 \n");
    offset += sizeof(int32_t);

    memcpy(stream + offset, &pcb->program_counter, sizeof(int32_t));
    offset += sizeof(int32_t);

    buffer->stream = stream;

    return buffer;
}



PCB *deserializar_pcb(BUFFER *buffer)
{
    PCB *pcb = malloc(sizeof(PCB));

    void *stream = buffer->stream;

    memcpy(&(pcb->PID), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(pcb->program_counter), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    return pcb;
}


BUFFER *serializar_instrucciones(Instruccion2 *instrucciones)
{
    BUFFER* buffer = malloc(sizeof(BUFFER));

      // Calcula el tamaño total necesario para la serialización
      buffer->size = sizeof(int32_t) * 11       
                  + strlen(instrucciones->valorChar) + 1
                  + strlen(instrucciones->nombreInstruccion) + 1
                  + strlen(instrucciones->registro) + 1
                  + strlen(instrucciones->nombreArchivo) + 1
                  + strlen(instrucciones->recurso) + 1; 

      void* stream = malloc(buffer->size);
      int offset = 0; // Desplazamiento

    // Copia cada miembro de la estructura en el búfer

    //NombreInstruccion long y palabra
    memcpy(stream + offset, &(instrucciones->nombreInstruccion_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instrucciones->nombreInstruccion, strlen(instrucciones->nombreInstruccion) + 1);
    offset += strlen(instrucciones->nombreInstruccion) + 1;

    //Valor
    memcpy(stream + offset, &(instrucciones->valor), sizeof(int32_t));
    offset += sizeof(int32_t);

    //ValorChar longitud y palabra
    memcpy(stream + offset, &(instrucciones->valorChar_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instrucciones->valorChar, strlen(instrucciones->valorChar) + 1);
    offset += strlen(instrucciones->valorChar) + 1;

    //Registro longitud y palabra
    memcpy(stream + offset, &(instrucciones->registro_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instrucciones->registro, strlen(instrucciones->registro) + 1);
    offset += strlen(instrucciones->registro) + 1;

    memcpy(stream + offset, &(instrucciones->direccionLogica), sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, &(instrucciones->tiempo), sizeof(int32_t));
    offset += sizeof(int32_t);

    //nombreArchivo longitud y palabra
    memcpy(stream + offset, instrucciones->nombreArchivo, strlen(instrucciones->nombreArchivo) + 1);
    offset += strlen(instrucciones->nombreArchivo) + 1;
    memcpy(stream + offset, &(instrucciones->nombreARchivoLength), sizeof(int32_t));
    offset += sizeof(int32_t);


    memcpy(stream + offset, &(instrucciones->posicion), sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, &(instrucciones->cantBytes), sizeof(int32_t));
    offset += sizeof(int32_t);

    //Recurso longitud y palabra
    memcpy(stream + offset, &(instrucciones->recurso_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instrucciones->recurso, strlen(instrucciones->recurso) + 1);
    offset += strlen(instrucciones->recurso) + 1;

    memcpy(stream + offset, &(instrucciones->idSegmento), sizeof(int32_t));

    // Guarda el tamaño y los datos serializados en la estructura BUFFER
    buffer->stream = stream;

    return buffer;
}

Instruccion2* deserializar_instrucciones(BUFFER* buffer)
{
    Instruccion2* instrucciones = (Instruccion2*)malloc(sizeof(Instruccion2));
    void* stream = buffer->stream;


    // Lee cada miembro de la estructura desde el búfer

    memcpy(&(instrucciones->nombreInstruccion_long), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instrucciones->nombreInstruccion = malloc(instrucciones->nombreInstruccion_long);
    memcpy(instrucciones->nombreInstruccion, stream, instrucciones->nombreInstruccion_long);
    stream += instrucciones->nombreInstruccion_long + 1;

    memcpy(&(instrucciones->valor), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instrucciones->valorChar), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instrucciones->valorChar = malloc(instrucciones->valorChar_long);
    memcpy(instrucciones->valorChar, stream, instrucciones->valorChar_long);
    stream += instrucciones->valorChar_long + 1;

    memcpy(&(instrucciones->registro), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instrucciones->registro = malloc(instrucciones->registro_long);
    memcpy(instrucciones->registro, stream, instrucciones->registro_long);
    stream += instrucciones->registro_long+ 1;

    memcpy(&(instrucciones->direccionLogica), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instrucciones->tiempo), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instrucciones->nombreARchivoLength), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instrucciones->nombreArchivo = malloc(instrucciones->nombreARchivoLength);
     memcpy(instrucciones->nombreArchivo, stream, instrucciones->nombreARchivoLength);
    stream += instrucciones->nombreARchivoLength + 1;

    memcpy(&(instrucciones->posicion), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instrucciones->cantBytes), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instrucciones->recurso), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instrucciones->recurso = malloc(instrucciones->recurso_long);
    memcpy(instrucciones->recurso, stream, instrucciones->recurso_long);
    stream += instrucciones->recurso_long + 1;

    memcpy(&(instrucciones->idSegmento), stream, sizeof(int32_t));

    return instrucciones;
}



