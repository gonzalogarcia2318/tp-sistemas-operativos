#include "tipos.h"

// BUFFER *serializar_pcb(PCB *pcb)
// {
//     BUFFER *buffer = malloc(sizeof(PCB));

//     buffer->size = sizeof(int32_t) * 2;

//     void *stream = malloc(buffer->size);
//     int offset = 0;

//     memcpy(stream + offset, pcb->pID, sizeof(int32_t));
//     offset += sizeof(int32_t);
//     memcpy(stream + offset, pcb->program_counter, sizeof(int32_t));
//     offset += sizeof(int32_t);

//     buffer->stream = stream;

//     return buffer;
// }

// PCB *deserializar_pcb(BUFFER *buffer)
// {
//     PCB *pcb = malloc(sizeof(PCB));

//     void *stream = buffer->stream;

//     memcpy(&(pcb->pID), stream, sizeof(int32_t));
//     stream += sizeof(int32_t);
//     memcpy(&(pcb->program_counter), stream, sizeof(int32_t));
//     stream += sizeof(int32_t);

//     return pcb;
// }