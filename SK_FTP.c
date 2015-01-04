#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define QSIZE 5
#define BUFSIZE 10000

char* r_220="220 Service ready for new user.\n";
char* r_215="215 Unix sytem type.\n";
char* r_331="331 User name okay, need password.\n";
char* r_230="230 User logged in, proceed.\n";
char* r_200="200 Command okay.\n";
char* r_500="500 Syntax error, command unrecognized.\n";
char* r_257="257 /usr/Xanril/ directory.\n";
char* r_227="227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).\n";
char* r_502="502 Command not implemented.\n";

char *protocol = "tcp";

ushort service_port = 21;
ushort data_port = 20;

pthread_t main_thread;

void serv_command(void* arg)
{
char command_buf[256];  
read((int) arg,command_buf,256);
strcat(command_buf," > res.txt");
printf("%s\n",command_buf);
system(command_buf);
FILE *fp;
fp = fopen("res.txt", "r"); 
  if(NULL == fp)
  {
  printf("Error opening file");
  exit(EXIT_FAILURE);
  } 
while(1)
{
unsigned char buff[256]={0};
int nread = fread(buff,1,256,fp);    

if(nread > 0)
{                           
write((int) arg, buff, nread);
}
if(nread <256)break;
}
close((int)fp);
}

int respond(void* arg)
{
char buffer[256]=" ";
char* command;
char* atribut;
int status=0;
read((int) arg,buffer,256);
command=strtok(buffer," ");
atribut=strtok(NULL," ");
if(strcmp(command,"USER")!=0 && strcmp(command,"PORT")!=0){command[strlen(command)-2]='\0';printf("%s",command);}
if(strcmp(command,"USER")==0){write((int) arg,r_230,strlen(r_230));printf("%s Response 230\n",command);}
else if(strcmp(command,"PORT")==0){write((int) arg,r_200,strlen(r_200));printf("%s Response 200\n",command);}
else if(strcmp(command,"SYST")==0){write((int) arg,r_215,strlen(r_215));printf(" Respons 215\n");}
else if(strcmp(command,"PWD")==0){write((int) arg,r_257,strlen(r_257));printf(" Respons 257\n");}
else if(strcmp(command,"PASV")==0){write((int) arg,r_227,strlen(r_227));printf(" Respons 227\n");}
else if(strcmp(command,"QUIT")==0){status=1;printf(" Respons quit\n");}
else if(strcmp(command,"LIST")==0){write((int) arg,r_502,strlen(r_502));printf(" Respons 502\n");}
else {write((int) arg,r_500,strlen(r_500));printf(" Response 500\n");}
return status;
}

void send_file(char* file_name,int arg)
{
FILE *fp;
fp = fopen(file_name, "rb"); 
  if(NULL == fp)
  {
  printf("Error opening file");
  exit(EXIT_FAILURE);
  } 
while(1)
{
unsigned char buff[256]={0};
int nread = fread(buff,1,256,fp);    

if(nread > 0)
{                           
write((int) arg, buff, nread);
}

if (nread < 256)
{
  break;
}
}
close((int)fp);
}

void* function(void* arg)
{
int stat=0;
printf("Connection established..\n");
write((int) arg,r_220,strlen(r_220));
while(stat!=1)
{
stat=respond(arg);
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
if(pthread_create (&main_thread,NULL,main_loop,NULL) !=0){
   printf("Thread creation error\n");
   exit(EXIT_FAILURE);
  }
printf("Server runing...\n");
printf("Press <ENTER> to terminate\n");
getc(stdin);
exit(EXIT_SUCCESS);
}
