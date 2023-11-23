#include<ucontext.h>
#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include"4_1_coro.h"

co_t *curr_context;
co_t *prev_context;

int mycoroutines_init(co_t *main) {
    ucontext_t context = {0};
    main->context = context;
    curr_context = main;
    main->is_co_finished = 0;
    if (getcontext(&main->context) == -1) {
        printf("getcontext error %d\n",__LINE__);
        return(0);
    }
    return(1);
}

int mycoroutines_create(co_t *co,void (body)(void *),void *arg) {

    ucontext_t context = {0};
    co->context = context;
    co->is_co_finished = 0;
    if (getcontext(&co->context) == -1) {
        printf("getcontext error %d\n",__LINE__);
        return(0);
    }

    co->context.uc_stack.ss_sp = calloc(1,SIGSTKSZ);
    if (co->context.uc_stack.ss_sp == NULL) {
        printf("Calloc failed at %d\n",__LINE__);
    }
    co->context.uc_stack.ss_size = SIGSTKSZ;
    co->context.uc_stack.ss_flags = 0;

    co->context.uc_link = 0;

    makecontext(&co->context,(void(*)())body,1,arg);

    return(1);
}

int mycoroutines_switchto (co_t *co) {
    if (co->is_co_finished == 1) {
        return(0);
    }

    prev_context = curr_context;
    curr_context = co;

    if (swapcontext(&prev_context->context,&curr_context->context) == -1) {
        printf("swapcontext error %d\n",__LINE__);
        return(0);
    }
    return(1);
}

int mycoroutines_destroy(co_t *co) {
    co->is_co_finished = 1;
    free(co->context.uc_stack.ss_sp);
    return(1);
}

