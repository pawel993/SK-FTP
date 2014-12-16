#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define QSIZE 5
#define BUFSIZE 10000
char *protocol = "tcp";
ushort service_port = 4000;

char *response="Hello, this is diagnostic service\n";

pthread_t main_thread;

void* function(void* arg)
{
  int ile;
  char buffer[10000];
while(buffer[0]!='.')
  {
  ile=read((int) arg,buffer,10000);
  write((int) arg,buffer,ile); 
  }
 close((int) arg);
}

void* main_loop(void* arg)
{
  struct sockaddr_in server_addr,client_addr;
  
  int sck,rcv_sck,rcv_len;
  
  bzero(&server_addr, sizeof server_addr);
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_family= AF_INET;
  server_addr.sin_port = htons(service_port);
  
  if ((sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
    perror("Cannot open socket");

    exit(EXIT_FAILURE);
  }
  
  if (bind(sck,(struct sockaddr*)&server_addr, sizeof server_addr) <0){
    printf("Cannot bind socket %d to %d port\n",sck,service_port);
    exit(EXIT_FAILURE);
  }
  
  if (listen (sck,QSIZE)<0){
    perror("Cannot listen");
    exit(EXIT_FAILURE);
  }
  
while(1){
  rcv_len = sizeof (struct sockaddr_in);
  if ((rcv_sck = accept(sck,(struct sockaddr*) &client_addr, (socklen_t*) &rcv_len)) < 0){
    perror("Error while connecting with client");
    exit(EXIT_FAILURE);
  }
  if(pthread_create (&main_thread,NULL,function,(void*) rcv_sck) !=0){
   printf("Thread creation error\n");
  exit(EXIT_FAILURE); 
  } 
  
}
  

  
 
  close(sck);
}
int main(int argc,char *argv[])
{
 char cmd[100]="ls -al";
  int a;
/*if(pthread_create (&main_thread,NULL,main_loop,NULL) !=0){
   printf("Thread creation error\n");
   exit(EXIT_FAILURE);
  }
printf("Server runing...\n");
printf("Press <ENTER> to terminate\n");
getc(stdin);*/
system(cmd);
exit(EXIT_SUCCESS);
}