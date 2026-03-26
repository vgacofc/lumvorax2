#ifndef MILLENNIUM_SOLVER_H
#define MILLENNIUM_SOLVER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct millennium_context millennium_context_t;

millennium_context_t* millennium_solver_init(void);
void millennium_solver_destroy(millennium_context_t* ctx);
bool millennium_solver_verify(millennium_context_t* ctx);

#endif
