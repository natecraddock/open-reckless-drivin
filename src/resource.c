#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "resource.h"

struct Resource {
  char type[8];
  uint32_t id;
  uint32_t length;
};

/* Search through the packed resource data for the given resource. */
static void *FindResource(const char *type, short id, uint32_t *len) {
  uint8_t *resource_ptr = _binary_data_start;
  while (resource_ptr != _binary_data_end) {
    struct Resource *resource = (struct Resource *)resource_ptr;

    if ((strcmp(type, resource->type) == 0) && id == resource->id) {
      *len = resource->length;
      return resource_ptr + sizeof(struct Resource);
    }

    resource_ptr += sizeof(struct Resource) + resource->length;
  }

  return NULL;
}

Handle GetResource(const char *type, short id) {
  uint32_t length;
  char *data = FindResource(type, id, &length);
  if (data == NULL) {
    return NULL;
  }

  Pointer *ptr = malloc(sizeof *ptr);
  if (!ptr) {
    return NULL;
  }

  ptr->size = length;
  ptr->data = data;

  return &ptr->data;
}

void ReleaseResource(Handle resource) {
  char *data = (char *)resource;
  Pointer *ptr = (Pointer *)(data - 8);
  free(*resource);
  free(ptr);
}

void PtrToHandle(Ptr src, Handle *dst, uint32_t size) {
  char *p = malloc(5);
  char *data = malloc(size);
  memcpy(data, src, size);
  Pointer *ptr = malloc(sizeof *ptr);
  ptr->size = size;
  ptr->data = data;
  *dst = &ptr->data;
}

int GetHandleSize(Handle handle) {
  char *data = (char *)handle;
  Pointer *ptr = (Pointer *)(data - 8);

  return ptr->size;
}

void SetHandleSize(Handle handle, int size) {
  char *data = (char *)handle;
  Pointer *ptr = (Pointer *)(data - 8);

  ptr->size = size;
}

void DisposeHandle(Handle handle) {
  char *data = (char *)handle;
  Pointer *ptr = (Pointer *)(data - 8);
  free(data);
  free(ptr);
}
