#include "helper_functions.h"
#include "server_main.c"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

int main(int argc,char *argv[])
{
  system("mkdir Files");
  initialize_ports(49160, 54000);
  pthread_mutex_init(&lock, NULL);
  pthread_mutex_init(&lock2, NULL);
  if(pthread_create (&main_thread,NULL,main_loop,NULL) !=0)
  {
    printf("Thread creation error\n");
    exit(EXIT_FAILURE);
  }
  printf("Server runing...\n");
  printf("Press <ENTER> to terminate\n");
  getc(stdin);
  exit(EXIT_SUCCESS);
}