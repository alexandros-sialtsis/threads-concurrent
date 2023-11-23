
#ifndef __4_1_H_
#define __4_1_H_

#include <ucontext.h>

struct co_t_ {
    ucontext_t context;
    int is_co_finished;
};

typedef struct co_t_ co_t;

int mycoroutines_init(co_t *main);

int mycoroutines_create(co_t *co,void (body)(void *),void *arg);

int mycoroutines_switchto (co_t *co);

int mycoroutines_destroy(co_t *co);

#endif
