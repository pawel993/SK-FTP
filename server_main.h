#ifndef _SERVER_MAIN_H
#define _SERVER_MAIN_H
#include <pthread.h>
/*
 * SERVER_MAIN
 * główne funkcje serwera
 */


extern pthread_t main_thread;
extern pthread_mutex_t lock,lock2;
extern void* main_loop(void* arg);

#endif