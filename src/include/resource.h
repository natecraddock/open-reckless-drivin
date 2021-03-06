#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdlib.h>

#include "defines.h"

/* Linked with data.o */
extern unsigned char _binary_data_start[];
extern const unsigned char _binary_data_end[];
extern unsigned _binary_data_size;

/* Get a resource. */
Handle GetResource(const char *type, short id);

/* Free a resource. */
void ReleaseResource(Handle resource);

/* Print resource data. */
void PrintResource();

/* TODO: These should be moved */

void PtrToHandle(Ptr src, Handle *dst, uint32_t size);

int GetHandleSize(Handle handle);
void SetHandleSize(Handle handle, int size);

#endif /* RESOURCE_H */
