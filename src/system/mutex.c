# include "mutex.h"
void extern _ffly_mutex_lock(ffly_mutex_t*);
void extern _ffly_mutex_unlock(ffly_mutex_t*);
// make sure correct register is used.
void ffly_mutex_lock(ffly_mutex_t *__m) {
	__asm__("mov %0, %%rdi\n\t"
        "call _ffly_mutex_lock" : : "r"(__m));
}

void ffly_mutex_unlock(ffly_mutex_t *__m) {
    __asm__("mov %0, %%rdi\n\t" 
        "call _ffly_mutex_unlock" : : "r"(__m));
}

/*
# include <pthread.h>
# include <stdio.h>
ffly_mutex_t m = FFLY_MUTEX_INIT;
mdl_uint_t x = 0;
void* test(void* __arg) {
    mdl_uint_t i = 0;
    while(i++ != 20000) {
    ffly_mutex_lock(&m);
        x++;
    ffly_mutex_unlock(&m);
    }
}

int main() {
    pthread_t a,b;
    pthread_create(&a, NULL, test, NULL);
    pthread_create(&b, NULL, test, NULL);
    pthread_join(a, NULL);
    pthread_join(b, NULL);
    printf("%u\n", x);
    //ffly_mutex_lock(&m);
  //  ffly_mutex_lock(&m);
//    printf("%u\n", m);
}*/