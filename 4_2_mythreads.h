
#ifndef __4_2_MYTHREADS_H_
#define __4_2_MYTHREADS_H_

#include <stdio.h>

/*
Every thead must start with mythreads_start()
and end with mythreads_finish()
*/

struct my_threads {
    co_t co;
    int position;
    int is_finished;
    int is_blocked;
};

typedef struct my_threads thr_t;

/*
Positions:
    'i' : integer
    'c' : character
    'd' : double
    's' : string
    'n' : pointer to integer
    'h' : pointer to character
    'o' : pointer to double
    't' : pointer to string
*/

struct tuple {
    int size;

    int *integers;
    int **integers_ptrs;

    char *characters;
    char **characters_ptrs;

    double *doubles;
    double **doubles_ptrs;

    char **strings;
    char **strings_ptrs;

    char *positions;
};

typedef struct tuple tuple_t;


/* Thread functions */

extern int mythreads_init();

extern int mythreads_create(thr_t *thr,void(body)(void*),void *arg);

extern int mythreads_yield();

extern int mythreads_join(thr_t *thr);

extern int mythreads_destroy(thr_t *thr);

extern void mythreads_start();

extern void mythreads_finish();


/* Tuple functions */

extern int mythreads_tuple_out(char *fmt, ...);

extern int string_analysis(va_list argp,char *fmt,tuple_t *tuple);

extern int mythreads_tuple_in(char *fmt,...);

int tuple_compare(tuple_t *temp, tuple_t *tuple);

void mythreads_tuple_delete(tuple_t *tuple);

#endif
