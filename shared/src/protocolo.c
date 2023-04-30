#include "protocolo.h"

char* mi_funcion_compartida(){
    return "Hice uso de la shared!";
}

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
    mensaje = string_duplicate((char *)list_get(listaMensaje, 0));
    list_destroy_and_destroy_elements(listaMensaje, &free);
    break;

  default:
    break;
  }

  return mensaje;
}