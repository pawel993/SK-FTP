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
char* current_dir;
char* mode="ACTIVE";
char* r_220="220 Service ready for new user.\n";
char* r_215="215 Unix system type.\n";
char* r_331="331 User name okay, need password.\n";
char* r_230="230 User logged in, proceed.\n";
char* r_200="200 Command okay.\n";
char* r_500="500 Syntax error, command unrecognized.\n";
char* r_257="257 /usr/Xanril/ directory.\n";
char* r_227="227 Entering Passive Mode (127,0,0,1,4,21).\n";
char* r_502="502 Command not implemented.\n";
char* r_150="150 File status okay; about to open data connection.\n";
char* r_125="125 Data connection already open; transfer starting.\n";
char* r_226="226 Closing data connection.\n";

char *protocol = "tcp";

ushort service_port = 21;
ushort passive_port = 1045;
ushort data_port = 20;

pthread_t main_thread;



void create_dir(char* directory_name)
{
 printf("Directory %s create\n",directory_name);
 char cmd[6]="mkdir ";
 strcat(cmd,directory_name);
 printf("%s",cmd);
 system(cmd);
}

void ls(void* arg)
{
system("ls -l > res.txt");
FILE* fp;
fp = fopen("res.txt", "rb"); 
  if(NULL == fp)
  {
  printf("Error opening file");
  exit(EXIT_FAILURE);
  } 
if(strcmp(mode,"PASSIVE")==0)
{
  printf(" PASSIVE MODE LS\n");
  struct sockaddr_in serv_data,client_data;
  int data_sck,cli_data,cli_len;
  bzero(&serv_data, sizeof serv_data);
  serv_data.sin_addr.s_addr = INADDR_ANY;
  serv_data.sin_family= AF_INET;
  serv_data.sin_port = htons(passive_port);
  
  if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
    perror("Cannot open socket");
    exit(EXIT_FAILURE);
  }
 
 
  if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0){
    printf("Cannot bind socket %d to %d port\n",data_sck,passive_port);
    exit(EXIT_FAILURE);
  }
  write((int) arg,r_150,strlen(r_150));printf(" Respons 150\n");
  
  if (listen (data_sck,QSIZE)<0){
    perror("Cannot listen");
    exit(EXIT_FAILURE);
  }
  
  
  printf("Waiting...\n");
  
  cli_len = sizeof (struct sockaddr_in);
  if ((cli_data = accept(data_sck,(struct sockaddr*) &client_data, (socklen_t*) &cli_len)) < 0){
    perror("Error while connecting with client");
    exit(EXIT_FAILURE);
  }  
    printf("Data connection established\n");
  while(1)
  {
  unsigned char buff[256]={0};
  int nread = fread(buff,8,256,fp);    
  if(nread > 0)
  {        
  printf("%s\n",buff);
  write(cli_data, buff, nread);	
  }
  if(nread <256)break;
  }
  close((int)fp);
  write((int) arg,r_226,strlen(r_226));
  close(data_sck);
  close(cli_data);
  }
else
{
}
}

void addr(char* addres)
{
char temp[48];
char* port;
int i=0;
int count=0;
int count2=0;
strncpy(temp,addres,48);
for(i=0;i<strlen(addres)-1;i++)
{
if(temp[i]==44)
{
count++;
temp[i]=46;
}
if(count<4){count2++;}
}
char ad[count2];
strncpy(ad,temp,count2);
printf("%s   ||\n",ad);
}

int respond(void* arg)
{
char buffer[256]=" ";
char* command;
char* atribut;
int status=0;
read((int) arg,buffer,256);
printf("%s",buffer);
command=strtok(buffer," ");
atribut=strtok(NULL," ");
if(strcmp(command,"USER")!=0 && strcmp(command,"PORT")!=0 && strcmp(command,"MKD")!=0 && strcmp(command,"TYPE")!=0 )
{command[strlen(command)-2]='\0';printf("%s",command);}
if(strcmp(command,"USER")==0)
{write((int) arg,r_230,strlen(r_230));printf("%s Response 230\n",command);}
else if(strcmp(command,"MKD")==0)
{atribut[strlen(atribut)-2]='\0';create_dir(atribut);write((int) arg,r_257,strlen(r_257));printf("%s %s Response 257\n",command,atribut);}
else if(strcmp(command,"PORT")==0)
{atribut[strlen(atribut)-2]='\0';write((int) arg,r_200,strlen(r_200));printf("%s Response 200\n",command);addr(atribut);}
else if(strcmp(command,"SYST")==0)
{write((int) arg,r_215,strlen(r_215));printf(" Respons 215\n");}
else if(strcmp(command,"PWD")==0)
{write((int) arg,r_257,strlen(r_257));printf(" Respons 257\n");}
else if(strcmp(command,"PASV")==0)
{write((int) arg,r_227,strlen(r_227));printf(" Respons 227\n");mode="PASSIVE";ls(arg);}
else if(strcmp(command,"QUIT")==0)
{status=1;printf(" Respons quit\n");}
else if(strcmp(command,"LIST")==0)
{ls(arg);}
else if(strcmp(command,"TYPE")==0)
{write((int) arg,r_200,strlen(r_200)),printf(" Respons 200\n");}
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
