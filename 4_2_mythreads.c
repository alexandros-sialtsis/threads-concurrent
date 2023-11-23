
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include "4_1_coro.h"
#include "4_2_mythreads.h"

#define SEC_TIMER 0
#define USEC_TIMER 500

/*Threads*/

int thread_count = 0;
int current_thread = 0;
thr_t main_thread;
thr_t **thread_table;
struct itimerval timer, stop_timer;

/*Tuples*/

tuple_t **tuple_space;
int tuple_space_size;




/* Thread functions */

int next_thread(int current_thread) {
    do {
        if (current_thread == thread_count - 1) {
            current_thread = 0;
        }
        else {
            current_thread++;
        }
    } while (thread_table[current_thread]->is_finished == 1);
    return(current_thread);
}

void thread_handler() {
    printf("thread %d entered the handler (thread_count = %d)\n", current_thread, thread_count);
    if (thread_count != 1) {
        current_thread = next_thread(current_thread);
        printf("(new current_thread = %d)\n", current_thread);
        mycoroutines_switchto(&thread_table[current_thread]->co);
    }
    // - - - - - - - - - - - - - - - - - - - - - 
    printf("(resuming to thread %d\n", current_thread);
    sleep(2);
    setitimer(ITIMER_REAL, &timer, NULL);
    printf("resuming\n");
}



int mythreads_init() {
    thread_table = (thr_t **) calloc(1, sizeof(thr_t *));
    if(thread_table == NULL) {
        printf("error at calloc at %d\n", __LINE__);
        return(0);
    }
    thread_table[0] = &main_thread;
    if(mycoroutines_init(&thread_table[0]->co) == 0) return(0);
    thread_count++;
    thread_table[0]->position = 0;
    thread_table[0]->is_blocked = 0;
    thread_table[0]->is_finished = 0;

    signal(SIGALRM, &thread_handler);

    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = SEC_TIMER;
    timer.it_value.tv_usec = USEC_TIMER;

    stop_timer.it_interval.tv_sec = 0;
    stop_timer.it_interval.tv_usec = 0;
    stop_timer.it_value.tv_sec = 0;
    stop_timer.it_value.tv_usec = 0;

    thread_handler();

    return(1);
}

int mythreads_create(thr_t *thr, void (body)(void *), void *arg) {
    if(mycoroutines_create(&thr->co, (void(*)())body, arg) == 0) return(0);
    thread_count++;
    thr->is_blocked = 0;
    thr->is_finished = 0;
    thr->position = thread_count - 1;
    thread_table = (thr_t **) realloc(thread_table, thread_count * sizeof(thr_t *));
    if(thread_table == NULL) {
        printf("error at realloc at %d\n", __LINE__);
        return(0);
    }
    thread_table[thr->position] = thr;
    return(1);
}

int mythreads_yield() {
    if(thread_count == 1) return(0);

    current_thread = next_thread(current_thread);
    printf("(new current_thread = %d)\n", current_thread);
    mycoroutines_switchto(&thread_table[current_thread]->co);
    // - - - - - - - - - - - - - - - - - - - - - 
    printf("resuming from yield\n");
    setitimer(ITIMER_REAL, &timer, NULL);
    return(1);
}

int mythreads_join(thr_t *thr) {
    if (thread_count == 1) return(0);

    setitimer(ITIMER_REAL,&stop_timer,NULL);

    while (thr->is_finished != 1) {
        thread_table[current_thread]->is_blocked = 1;
        current_thread = next_thread(current_thread);
        mycoroutines_switchto(&thread_table[current_thread]->co);
    }

    thread_table[current_thread]->is_blocked = 0;
    setitimer(ITIMER_REAL,&timer,NULL);
    return(1);
}

int mythreads_destroy(thr_t *thr) {
    if(thr->is_finished == 1) return(0);

    setitimer(ITIMER_REAL,&stop_timer,NULL);

    thread_table[current_thread]->is_finished = 1;
    mycoroutines_destroy(&thread_table[current_thread]->co);
    return(1);
}

void mythreads_start() {
    setitimer(ITIMER_REAL, &timer, NULL);
}

void mythreads_finish() {
    setitimer(ITIMER_REAL,&stop_timer,NULL);
    thread_table[current_thread]->is_finished = 1;
    thread_handler();
}




/* Tuple functions */

int mythreads_tuple_out(char *fmt, ...) {
    int i;
    va_list argp;
    struct itimerval resume_timer;

    setitimer(ITIMER_REAL,&stop_timer,&resume_timer);

    for (i = 0; i < tuple_space_size; i++) {
        if (tuple_space[i] == NULL) {
            break;
        }
    }
    if (i == tuple_space_size) {
        tuple_space_size++;
        tuple_space = realloc(tuple_space,tuple_space_size * sizeof(tuple_t*));
        if (tuple_space == NULL) {
            printf("Realloc error at %d\n",__LINE__);
        }
    }
    tuple_space[i] = calloc(1,sizeof(tuple_t));
    if (tuple_space[i] == NULL) {
        printf("calloc error at %d\n",__LINE__);
    }
    va_start(argp,fmt);
    string_analysis(argp,fmt,tuple_space[i]);

    setitimer(ITIMER_REAL,&resume_timer,NULL);

    return(1);
}

int string_analysis(va_list argp,char *format,tuple_t *tuple) {
    int i = 0, position_index = 0, integer_index = 0, string_index = 0 , character_index = 0, double_index = 0;
    int integer_ptrs_index = 0, string_ptrs_index = 0 , character_ptrs_index = 0, double_ptrs_index = 0;
    while (format[i] != '\0') {
        if (format[i] == '%') {
            i++;
            if (format[i] == 'd') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 'i';
                position_index++;

                tuple->integers = realloc(tuple->integers,(integer_index + 1) * sizeof(int));
                if (tuple->integers == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->integers[integer_index] = va_arg(argp,int);
                integer_index++;
            }

            else if (format[i] == 'c') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 'c';
                position_index++;

                tuple->characters = realloc(tuple->characters,(character_index + 1) * sizeof(char));
                if (tuple->characters == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->characters[character_index] = va_arg(argp,int);
                character_index++;
            }
            else if (format[i] == 'l' && format[i+1] == 'f') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 'd';
                position_index++;

                tuple->doubles = realloc(tuple->doubles,(double_index + 1) * sizeof(double));
                if (tuple->doubles == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->doubles[double_index] = va_arg(argp,double);
                double_index++;
                i++;
            }
            else if (format[i] == 's') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 's';
                position_index++;

                tuple->strings = realloc(tuple->strings,(string_index + 1) * sizeof(char*));
                if (tuple->strings == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->strings[string_index] = calloc(50,sizeof(char));
                if (tuple->strings[string_index] == NULL) {
                    printf("Calloc error at %d\n",__LINE__);
                }
                strcpy( tuple->strings[string_index], va_arg(argp,char*) );
                string_index++;
            }
        }
        else if (format[i] == '&') {
            i++;
            if (format[i] == 'n') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 'n';
                position_index++;

                tuple->integers_ptrs = realloc(tuple->integers_ptrs,(integer_ptrs_index + 1) * sizeof(int *));
                if (tuple->integers_ptrs == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->integers_ptrs[integer_ptrs_index] = va_arg(argp,int *);
                integer_ptrs_index++;
            }

            else if (format[i] == 'h') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 'h';
                position_index++;

                tuple->characters_ptrs = realloc(tuple->characters_ptrs,(character_ptrs_index + 1) * sizeof(char *));
                if (tuple->characters_ptrs == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->characters_ptrs[character_ptrs_index] = va_arg(argp,char *);
                character_ptrs_index++;
            }
            else if (format[i] == 'o') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 'o';
                position_index++;

                tuple->doubles_ptrs = realloc(tuple->doubles_ptrs,(double_ptrs_index + 1) * sizeof(double *));
                if (tuple->doubles_ptrs == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->doubles_ptrs[double_ptrs_index] = va_arg(argp,double *);
                double_ptrs_index++;
            }
            else if (format[i] == 't') {
                tuple->positions = realloc(tuple->positions,(position_index + 1) * sizeof(char));
                if (tuple->positions == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->positions[position_index] = 't';
                position_index++;

                tuple->strings_ptrs = realloc(tuple->strings_ptrs,(string_ptrs_index + 1) * sizeof(char*));
                if (tuple->strings_ptrs == NULL) {
                    printf("Realloc error at %d\n",__LINE__);
                }
                tuple->strings_ptrs[string_ptrs_index] = calloc(1,sizeof(char *));
                if (tuple->strings_ptrs[string_ptrs_index] == NULL) {
                    printf("Calloc error at %d\n",__LINE__);
                }
                tuple->strings_ptrs[string_ptrs_index] = va_arg(argp,char *);
                string_ptrs_index++;
            }
        }
        i++;
    }
    tuple->size = position_index;

    printf("tuple(size = %d) is:\n", tuple->size);
    for (i = 0; i < integer_index; i++) {
        printf("%d ",tuple->integers[i]);
    }
    for (i = 0; i < character_index; i++) {
        printf("%c ",tuple->characters[i]);
    }
    for (i = 0; i < double_index; i++) {
        printf("%lf ",tuple->doubles[i]);
    }
    for (i = 0; i < string_index; i++) {
        printf("%s ",tuple->strings[i]);
    }
    for (i = 0; i < integer_ptrs_index; i++) {
        printf("%p ",tuple->integers_ptrs[i]);
    }
    for (i = 0; i < character_ptrs_index; i++) {
        printf("%p ",tuple->characters_ptrs[i]);
    }
    for (i = 0; i < double_ptrs_index; i++) {
        printf("%p ",tuple->doubles_ptrs[i]);
    }
    for (i = 0; i < string_ptrs_index; i++) {
        printf("%p ",tuple->strings_ptrs[i]);
    }
    for (i = 0; i < position_index; i++) {
        printf("%c",tuple->positions[i]);
    }
    printf("\n");


    return(1);
}

int mythreads_tuple_in(char *fmt,...) {
    int i, ret = 0, has_blocked = 0;
    struct itimerval resume_timer;
    va_list argp;
    tuple_t *temp = calloc(1, sizeof(tuple_t));

    setitimer(ITIMER_REAL,&stop_timer,&resume_timer);

    va_start(argp, fmt);
    string_analysis(argp, fmt, temp);

    while(1) {
        for(i = 0; i < tuple_space_size; i++) {
            if(tuple_space[i] == NULL || temp->size != tuple_space[i]->size) continue;
            ret = tuple_compare(temp, tuple_space[i]);
            if(ret == 1) break;
        }
        if(ret == 1) break;
        has_blocked = 1;
        thread_table[current_thread]->is_blocked = 1;
        current_thread = next_thread(current_thread);
        mycoroutines_switchto(&thread_table[current_thread]->co);
    }

    thread_table[current_thread]->is_blocked = 0;
    mythreads_tuple_delete(tuple_space[i]);
    free(tuple_space[i]);
    tuple_space[i] = NULL;


    if(has_blocked == 0) setitimer(ITIMER_REAL, &resume_timer, NULL);
    else setitimer(ITIMER_REAL, &timer, NULL);
    return(1);
}

void mythreads_tuple_delete(tuple_t *tuple) {
    int i = 0, no_strings = 0;

    free(tuple->integers);
    free(tuple->characters);
    free(tuple->doubles);

    for(i = 0; i < tuple->size; i++) {
        if(tuple->positions[i] == 's') {
            no_strings++;
        }
    }
    for(i = 0; i < no_strings; i++) {
        free(tuple->strings[i]);
    }
    free(tuple->strings);
    free(tuple->positions);
}

int tuple_compare(tuple_t *temp, tuple_t *tuple) {
    int i = 0, integer_index = 0, string_index = 0 , character_index = 0, double_index = 0;
    int integer_index_temp = 0, string_index_temp = 0 , character_index_temp = 0, double_index_temp = 0;
    int integer_ptrs_index = 0, string_ptrs_index = 0 , character_ptrs_index = 0, double_ptrs_index = 0;

    for(i = 0; i < temp->size; i++) {
        if(temp->positions[i] == 'i') {
            if(tuple->positions[i] == 'i') {
                if(temp->integers[integer_index_temp] != tuple->integers[integer_index]) return(0);
                integer_index++;
                integer_index_temp++;
            }
            else{
                return(0);
            }
        }
        else if(temp->positions[i] == 'c') {
            if(tuple->positions[i] == 'c') {
                if(temp->characters[character_index_temp] != tuple->characters[character_index]) return(0);
                character_index++;
                character_index_temp++;
            }
            else {
                return(0);
            }
        }
        else if(temp->positions[i] == 'd') {
            if(tuple->positions[i] == 'd') {
                if(temp->doubles[double_index_temp] != tuple->doubles[double_index]) return(0);
                double_index++;
                double_index_temp++;
            }
            else {
                return(0);
            }
        }
        else if(temp->positions[i] == 's') {
            if(tuple->positions[i] == 's') {
                if(strcmp(temp->strings[string_index_temp], tuple->strings[string_index])) return(0);
                string_index++;
                string_index_temp++;
            }
            else {
                return(0);
            }
        }
        else if(temp->positions[i] == 'n') {
            if(tuple->positions[i] == 'i') {
                *(temp->integers_ptrs[integer_ptrs_index]) = tuple->integers[integer_index];
                integer_ptrs_index++;
                integer_index++;
            }
            else {
                return(0);
            }
        }
        else if(temp->positions[i] == 'h') {
            if(tuple->positions[i] == 'c') {
                *(temp->characters_ptrs[character_ptrs_index]) = tuple->characters[character_index];
                character_ptrs_index++;
                character_index++;
            }
            else {
                return(0);
            }
        }
        else if(temp->positions[i] == 'o') {
            if(tuple->positions[i] == 'd') {
                *(temp->doubles_ptrs[double_ptrs_index]) = tuple->doubles[double_index];
                double_ptrs_index++;
                double_index++;
            }
            else {
                return(0);
            }
        }
        else if(temp->positions[i] == 't') {
            if(tuple->positions[i] == 's') {
                strcpy(temp->strings_ptrs[string_ptrs_index], tuple->strings[string_index]);
                string_ptrs_index++;
                string_index++;
            }
            else {
                return(0);
            }
        }
    }
    return(1);
}

void *ttt1(void *arg) {
    int i;

    mythreads_start();


    //while(1) {
    for(i = 0; i < 200; i++) {
        printf("thread1111: %d\n", i);
        //mythreads_yield();
    }

    mythreads_finish();
    return(NULL);
}

void *ttt2(void *arg) {
    mythreads_start();


    //while(1) {
    for(int i = 0; i < 300; i++) {
        printf("thread2222: %d\n", i);
        //mythreads_yield();
        //usleep(10);
    }
    mythreads_finish();
    return(NULL);
}

void *ttt3(void *arg) {
    mythreads_start();

    //while(1) {
    for(int i = 0; i < 1000; i++) {
        printf("thread3333: %d\n", i);
        //mythreads_yield();
        //usleep(10);
    }
    mythreads_tuple_out("%d %d %c %c", 15, 100, 'c', 'h');
    mythreads_finish();
    return(NULL);
}

int main(int argc,char *argv[]) {
    int num;
    char c, str[50] = {'\0'};

    thr_t *thread1 = (thr_t *) malloc(sizeof(thr_t));
    thr_t *thread2 = (thr_t *) malloc(sizeof(thr_t));
    thr_t *thread3 = (thr_t *) malloc(sizeof(thr_t));

    mythreads_init();
    mythreads_create(thread1, (void(*)())ttt1, NULL);
    mythreads_create(thread2, (void(*)())ttt2, NULL);
    mythreads_create(thread3, (void(*)())ttt3, NULL);

    mythreads_tuple_out("%d %c %s %lf", 15,'f', "fifteen", 1.5);
    mythreads_tuple_out("%d %c %s %lf", 25,'t', "twenty_five", 2.5);

    mythreads_tuple_in("%d &n %c &h", 15, &num, 'c', &c);
    mythreads_tuple_in("%d %c &t %lf", 25,'t', str, 2.5);
    mythreads_tuple_in("%d &n %c &h", 15, &num, 'c', &c);

    printf("str = %s\n", str);

    mythreads_join(thread1);
    mythreads_join(thread2);
    mythreads_join(thread3);
    
    printf("destroying threads\n");
    mythreads_destroy(thread1);
    mythreads_destroy(thread2);
    mythreads_destroy(thread3);
    printf("goodbye\n");
    return(0);
}
