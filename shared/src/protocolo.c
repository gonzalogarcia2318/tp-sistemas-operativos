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
  BUFFER *buffer = recibir_buffer(socketCliente);
    
  t_list* instrucciones = deserializar_instrucciones(buffer);

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
  //void *buffer = obtener_buffer_del_cliente(&tamanioBuffer, socketCliente);
  BUFFER *buffer = recibir_buffer(socketCliente);

  while (desplazamiento < buffer->size)
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
    list_destroy(listaMensaje);
    //list_destroy_and_destroy_elements(listaMensaje, &free);
    break;

  default:
    break;
  }

  return mensaje;
}

PCB *obtener_paquete_pcb(int socket_cpu){
  BUFFER *buffer = recibir_buffer(socket_cpu);
    
  PCB* pcb = deserializar_pcb(buffer);

  free(buffer);

  return pcb;
}

CODIGO_INSTRUCCION obtener_codigo_instruccion(int socket_cliente)
{
  CODIGO_INSTRUCCION codigo_instruccion;

  if (recv(socket_cliente, &codigo_instruccion, sizeof(int), MSG_WAITALL) > 0)
    return codigo_instruccion;
  else
  {
    close(codigo_instruccion);
    return DESCONEXION;
  }
}

BUFFER *obtener_parametros_instruccion(int socket_cliente){
  BUFFER *buffer = recibir_buffer(socket_cliente);
  return buffer;
}


// TODO: MOVER A OTRO ARCHIVO


BUFFER *serializar_pcb(PCB *pcb)
{
    BUFFER *buffer = malloc(sizeof(PCB));

    buffer->size = sizeof(int32_t) * 4
                    + 4 * 4 + 4 * 8 + 4 * 16  // 4 registros de 4 bytes, 4 de 8, 4 de 16
                    + calcular_tamanio_instrucciones(pcb->instrucciones)
                    + calcular_tamanio_segmentos(pcb->tabla_segmentos);


    void *stream = malloc(buffer->size);
    int offset = 0;

    memcpy(stream + offset, &pcb->PID, sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, &pcb->program_counter, sizeof(int32_t));
    offset += sizeof(int32_t);

    BUFFER *buffer_instrucciones = serializar_instrucciones(pcb->instrucciones);
    memcpy(stream + offset, &buffer_instrucciones->size, sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, buffer_instrucciones->stream, buffer_instrucciones->size);
    offset += buffer_instrucciones->size;

    BUFFER *buffer_segmentos = serializar_segmentos(pcb->tabla_segmentos);
    memcpy(stream + offset, &buffer_segmentos->size, sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, buffer_segmentos->stream, buffer_segmentos->size);
    offset += buffer_segmentos->size;

    memcpy(stream + offset, &pcb->registros_cpu, (4*4+4*8+4*16));
    offset += (4*4+4*8+4*16);

    buffer->stream = stream;

    return buffer;
}

BUFFER *serializar_segmentos(t_list *segmentos){
    BUFFER* buffer = malloc(sizeof(BUFFER));
	buffer->size = 0;

	// Calcular tamaño total del buffer
	int i = 0;
	for(i = 0; i < list_size(segmentos); i++){
		SEGMENTO* segmento = list_get(segmentos, i);
        buffer->size += calcular_tamanio_segmento(segmento);
	}

	void* stream = malloc(buffer->size);
	int offset = 0;

	i = 0;
	for(i = 0; i < list_size(segmentos); i++){
		SEGMENTO* segmento = list_get(segmentos, i);
		BUFFER* buffer_segmento = serializar_segmento(segmento);
		memcpy(stream + offset, buffer_segmento->stream, buffer_segmento->size);
		offset += buffer_segmento->size;
	}

	buffer->stream = stream;

	return buffer;
}

BUFFER *serializar_segmento(SEGMENTO *segmento)
{
    BUFFER* buffer = malloc(sizeof(BUFFER));

    // Calcula el tamaño total necesario para la serialización
    buffer->size = calcular_tamanio_segmento(segmento);

    void* stream = malloc(buffer->size);
    int offset = 0; // Desplazamiento

    
    
    memcpy(stream + offset, &(segmento->base), sizeof(int32_t));
    offset += sizeof(int32_t);

 
    memcpy(stream + offset, &(segmento->id), sizeof(int32_t));
    offset += sizeof(int32_t);

    
    memcpy(stream + offset, &(segmento->limite), sizeof(int32_t));
    offset += sizeof(int32_t);
 

    // Guarda el tamaño y los datos serializados en la estructura BUFFER
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

    BUFFER* buffer_instrucciones = malloc(sizeof(BUFFER));
    memcpy(&(buffer_instrucciones->size), stream, sizeof(int32_t));
    stream += sizeof(int32_t);


    buffer_instrucciones->stream = stream;
    pcb->instrucciones = deserializar_instrucciones(buffer_instrucciones);
    stream += buffer_instrucciones->size;

    BUFFER* buffer_segmentos = malloc(sizeof(BUFFER));
    memcpy(&(buffer_segmentos->size), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    
    buffer_segmentos->stream = stream;
    pcb->tabla_segmentos = deserializar_segmentos(buffer_segmentos);
    stream += buffer_segmentos->size;

    memcpy(&(pcb->registros_cpu), stream, 112);
    stream += 112;


  
    free(buffer_segmentos);
    free(buffer_instrucciones);

    return pcb;
}

BUFFER *serializar_instruccion(Instruccion *instruccion)
{
    BUFFER* buffer = malloc(sizeof(BUFFER));

    // Calcula el tamaño total necesario para la serialización
    buffer->size = calcular_tamanio_instruccion(instruccion);

    void* stream = malloc(buffer->size);
    int offset = 0; // Desplazamiento

    // Copia cada miembro de la estructura en el búfer

    //NombreInstruccion long y palabra
    instruccion->nombreInstruccion_long = strlen(instruccion->nombreInstruccion);
    memcpy(stream + offset, &(instruccion->nombreInstruccion_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instruccion->nombreInstruccion, strlen(instruccion->nombreInstruccion) + 1);
    offset += strlen(instruccion->nombreInstruccion) + 1;

    //Valor
    memcpy(stream + offset, &(instruccion->valor), sizeof(int32_t));
    offset += sizeof(int32_t);

    //valor longitud y palabra
    instruccion->valor_long = strlen(instruccion->valor);
    memcpy(stream + offset, &(instruccion->valor_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instruccion->valor, strlen(instruccion->valor) + 1);
    offset += strlen(instruccion->valor) + 1;

    //Registro longitud y palabra
    instruccion->registro_long = strlen(instruccion->registro);
    memcpy(stream + offset, &(instruccion->registro_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instruccion->registro, strlen(instruccion->registro) + 1);
    offset += strlen(instruccion->registro) + 1;

    memcpy(stream + offset, &(instruccion->direccionLogica), sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, &(instruccion->tiempo), sizeof(int32_t));
    offset += sizeof(int32_t);

    //nombreArchivo longitud y palabra
    instruccion->nombreArchivo_long = strlen(instruccion->nombreArchivo);
    memcpy(stream + offset, &(instruccion->nombreArchivo_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instruccion->nombreArchivo, strlen(instruccion->nombreArchivo) + 1);
    offset += strlen(instruccion->nombreArchivo) + 1;


    memcpy(stream + offset, &(instruccion->posicion), sizeof(int32_t));
    offset += sizeof(int32_t);

    memcpy(stream + offset, &(instruccion->cantBytes), sizeof(int32_t));
    offset += sizeof(int32_t);

    //Recurso longitud y palabra
    instruccion->recurso_long = strlen(instruccion->recurso);
    memcpy(stream + offset, &(instruccion->recurso_long), sizeof(int32_t));
    offset += sizeof(int32_t);
    memcpy(stream + offset, instruccion->recurso, strlen(instruccion->recurso) + 1);
    offset += strlen(instruccion->recurso) + 1;

    memcpy(stream + offset, &(instruccion->idSegmento), sizeof(int32_t));

    // Guarda el tamaño y los datos serializados en la estructura BUFFER
    buffer->stream = stream;

    return buffer;
}

Instruccion* deserializar_instruccion(BUFFER* buffer, int stream_offset)
{
    Instruccion* instruccion = (Instruccion*)malloc(sizeof(Instruccion));
    void* stream = buffer->stream;
    stream += stream_offset;


    // Lee cada miembro de la estructura desde el búfer

    memcpy(&(instruccion->nombreInstruccion_long), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instruccion->nombreInstruccion = malloc(instruccion->nombreInstruccion_long+1); // +1 para el caracter nulo
    memcpy(instruccion->nombreInstruccion, stream, instruccion->nombreInstruccion_long);
    instruccion->nombreInstruccion[instruccion->nombreInstruccion_long] = '\0'; // Agrega el caracter nulo al final de la cadena
    stream += instruccion->nombreInstruccion_long + 1;


    memcpy(&(instruccion->valor), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instruccion->valor_long), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instruccion->valor = malloc(instruccion->valor_long+1);
    memcpy(instruccion->valor, stream, instruccion->valor_long);
    instruccion->valor[instruccion->valor_long] = '\0';
    stream += instruccion->valor_long + 1;

    memcpy(&(instruccion->registro_long), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instruccion->registro = malloc(instruccion->registro_long);
    memcpy(instruccion->registro, stream, instruccion->registro_long);
    instruccion->registro[instruccion->registro_long] = '\0';
    stream += instruccion->registro_long + 1;

    memcpy(&(instruccion->direccionLogica), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instruccion->tiempo), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instruccion->nombreArchivo_long), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instruccion->nombreArchivo = malloc(instruccion->nombreArchivo_long);
    memcpy(instruccion->nombreArchivo, stream, instruccion->nombreArchivo_long);
    instruccion->nombreArchivo[instruccion->nombreArchivo_long] = '\0';
    stream += instruccion->nombreArchivo_long + 1;

    memcpy(&(instruccion->posicion), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instruccion->cantBytes), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(instruccion->recurso_long), stream, sizeof(int32_t));
    stream += sizeof(int32_t);
    instruccion->recurso = malloc(instruccion->recurso_long);
    memcpy(instruccion->recurso, stream, instruccion->recurso_long);
    instruccion->recurso[instruccion->recurso_long] = '\0';
    stream += instruccion->recurso_long + 1;

    memcpy(&(instruccion->idSegmento), stream, sizeof(int32_t));

    return instruccion;
}

SEGMENTO* deserializar_segmento(BUFFER* buffer, int stream_offset)
{
    SEGMENTO* segmento = (SEGMENTO*)malloc(sizeof(SEGMENTO));
    void* stream = buffer->stream;
    stream += stream_offset;

    memcpy(&(segmento->id), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    memcpy(&(segmento->limite), stream, sizeof(int32_t));
    stream += sizeof(int32_t);


    memcpy(&(segmento->base), stream, sizeof(int32_t));
    stream += sizeof(int32_t);

    return segmento;
}

BUFFER *serializar_instrucciones(t_list *instrucciones){
    BUFFER* buffer = malloc(sizeof(BUFFER));
	buffer->size = 0;

	// Calcular tamaño total del buffer
	int i = 0;
	for(i = 0; i < list_size(instrucciones); i++){
		Instruccion* instruccion = list_get(instrucciones, i);
        buffer->size += calcular_tamanio_instruccion(instruccion);
	}

	void* stream = malloc(buffer->size);
	int offset = 0;

	i = 0;
	for(i = 0; i < list_size(instrucciones); i++){
		Instruccion* instruccion = list_get(instrucciones, i);
		BUFFER* buffer_instruccion = serializar_instruccion(instruccion);
		memcpy(stream + offset, buffer_instruccion->stream, buffer_instruccion->size);
		offset += buffer_instruccion->size;
	}

	buffer->stream = stream;

	return buffer;
}

t_list* deserializar_instrucciones(BUFFER* buffer){
	t_list* instrucciones = list_create();

	int size_instrucciones_acumulado = 0;
	do {
		Instruccion* instruccion = deserializar_instruccion(buffer, size_instrucciones_acumulado);
		size_instrucciones_acumulado += calcular_tamanio_instruccion(instruccion);
		list_add(instrucciones, instruccion);
	} while(size_instrucciones_acumulado < buffer->size);
	// Repetir mientras lo que ya se leyo no sea lo que trajo el buffer entero,
	// porque quiere decir que hay mas instrucciones por leer

	return instrucciones;
}

t_list* deserializar_segmentos(BUFFER* buffer){
	t_list* segmentos = list_create();

	int size_segmento_acumulado = 0;
	do {
		SEGMENTO* segmento = deserializar_segmento(buffer, size_segmento_acumulado);
		size_segmento_acumulado += calcular_tamanio_segmento(segmento);
		list_add(segmentos, segmento);
	} while(size_segmento_acumulado < buffer->size);
	// Repetir mientras lo que ya se leyo no sea lo que trajo el buffer entero,
	// porque quiere decir que hay mas instrucciones por leer

	return segmentos;
}

int calcular_tamanio_instruccion(Instruccion *instruccion){
    int tamanio = sizeof(int32_t) * 11       
            + strlen(instruccion->valor) + 1
            + strlen(instruccion->nombreInstruccion) + 1
            + strlen(instruccion->registro) + 1
            + strlen(instruccion->nombreArchivo) + 1
            + strlen(instruccion->recurso) + 1; 
    return tamanio;
}

int calcular_tamanio_segmento(SEGMENTO *segmento){
    int tamanio = sizeof(int32_t) * 3;                
    return tamanio;
}

int calcular_tamanio_instrucciones(t_list *instrucciones){
    int tamanio_total = 0;

    for(int i = 0; i < list_size(instrucciones); i++){
        tamanio_total += calcular_tamanio_instruccion(list_get(instrucciones, i));
    }

    return tamanio_total;
}

int calcular_tamanio_segmentos(t_list *segmentos){
    int tamanio_total = 0;

    for(int i = 0; i < list_size(segmentos); i++){
        tamanio_total += calcular_tamanio_segmento(list_get(segmentos, i));
    }

    return tamanio_total;
}

BUFFER *serializar_registros(Registro_CPU *registros)
{
    BUFFER* buffer = malloc(sizeof(BUFFER));

    buffer->size = 4*4 + 4*8 + 4*16;

    void* stream = malloc(buffer->size);
    int offset = 0;


    memcpy(stream + offset, &(registros->valor_AX), 4+1);
    offset += 4+1;
    memcpy(stream + offset, &(registros->valor_BX), 4+1);
    offset += 4+1;
    memcpy(stream + offset, &(registros->valor_CX), 4+1);
    offset += 4+1;
    memcpy(stream + offset, &(registros->valor_DX), 4+1);
    offset += 4+1;

    memcpy(stream + offset, &(registros->valor_EAX), 8+1);
    offset += 8+1;
    memcpy(stream + offset, &(registros->valor_EBX), 8+1);
    offset += 8+1;
    memcpy(stream + offset, &(registros->valor_ECX), 8+1);
    offset += 8+1;
    memcpy(stream + offset, &(registros->valor_EDX), 8+1);
    offset += 8+1;

    memcpy(stream + offset, &(registros->valor_RAX), 16+1);
    offset += 16+1;
    memcpy(stream + offset, &(registros->valor_RBX), 16+1);
    offset += 16+1;
    memcpy(stream + offset, &(registros->valor_RCX), 16+1);
    offset += 16+1;
    memcpy(stream + offset, &(registros->valor_RDX), 16+1);
    offset += 16+1;

    buffer->stream = stream;

    return buffer;
}

Registro_CPU *deserializar_registros(BUFFER *buffer)
{
    Registro_CPU *registros = (Registro_CPU*) malloc(sizeof(Registro_CPU));
    void* stream = buffer->stream;

    memcpy(&(registros->valor_AX), stream, 4+1);
    stream += 4+1;
    memcpy(&(registros->valor_BX), stream, 4+1);
    stream += 4+1;
    memcpy(&(registros->valor_CX), stream, 4+1);
    stream += 4+1;
    memcpy(&(registros->valor_DX), stream, 4+1);
    stream += 4+1;

    memcpy(&(registros->valor_EAX), stream, 8+1);
    stream += 8+1;
    memcpy(&(registros->valor_EBX), stream, 8+1);
    stream += 8+1;
    memcpy(&(registros->valor_ECX), stream, 8+1);
    stream += 8+1;
    memcpy(&(registros->valor_EDX), stream, 8+1);
    stream += 8+1;

    memcpy(&(registros->valor_RAX), stream, 16+1);
    stream += 16+1;
    memcpy(&(registros->valor_RBX), stream, 16+1);
    stream += 16+1;
    memcpy(&(registros->valor_RCX), stream, 16+1);
    stream += 16+1;
    memcpy(&(registros->valor_RDX), stream, 16+1);
    stream += 16+1;

    return registros;
}