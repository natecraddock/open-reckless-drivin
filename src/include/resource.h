#ifndef RESOURCE_H
#define RESOURCE_H

#include <stdlib.h>

#include "defines.h"

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
void DisposeHandle(Handle handle);

#endif /* RESOURCE_H */
