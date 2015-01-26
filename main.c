#include "helper_functions.h"
#include "server_main.c"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
/*
 * MAIN
 * FUNKCJA MAIN I START SERWERA
 * 
 */


int main(int argc,char *argv[])
{
  system("mkdir Files");//utworzenie katalogu głównego serwera
  initialize_ports(49160, 54000);//inicjalizacja początkowych wartości portów połączeń danych
  
  //inicjalizacja głównych semaforów
  pthread_mutex_init(&lock, NULL);
  pthread_mutex_init(&lock2, NULL);
  
  //stworzenie głównego wątku serwera
  if(pthread_create (&main_thread,NULL,main_loop,NULL) !=0)
  {
    printf("Thread creation error\n");
    exit(EXIT_FAILURE);
  }
  
  printf("Server runing...\n");
  printf("Press <ENTER> to terminate\n");
  
  //oczekiwanie na wciśnięcie klawisza enter
  getc(stdin);
  
  exit(EXIT_SUCCESS);
}