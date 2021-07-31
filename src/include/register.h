#ifndef __REGISTER_H
#define __REGISTER_H

#include <stdbool.h>
#include <stdint.h>

extern uint32_t gKey;
extern int gRegistered;

bool REG_check_registration();

#endif /* __REGISTER_H */
