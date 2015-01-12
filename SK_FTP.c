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
#include <signal.h>
#include <math.h>

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
int dd[2];
ushort passive_p;
ushort service_port = 21;
ushort data_port = 20;

pthread_t main_thread;

static void capture(int signo){
  printf("Signal captured\n");
  exit(EXIT_SUCCESS);
}
ushort passive_port()
{
passive_p++;
printf("Passive connection request.Prepering connection at port:%d\n",passive_p);
return passive_p;
}

void create_dir(char* directory_name)
{
 printf("Directory %s create\n",directory_name);
 char cmd[6]="mkdir ";
 strcat(cmd,directory_name);
 printf("%s",cmd);
 system(cmd);
}

void ls(int arg)
{
char tab[20]="lalalalala\n";
write((int)arg,tab,strlen(tab));
}

void passive_connection()
{
  struct sockaddr_in serv_data,client_data;
  int data_sck,cli_data,cli_len;
  bzero(&serv_data, sizeof serv_data);
  serv_data.sin_addr.s_addr = INADDR_ANY;
  serv_data.sin_family= AF_INET;
  serv_data.sin_port = htons(passive_port());
  
  if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
    perror("Cannot open socket");
    exit(EXIT_FAILURE);
  }
 
 
  if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0){
  printf("Cannot bind socket %d to %d port\n",data_sck,passive_port);
  exit(EXIT_FAILURE);
  }
  
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
    dd[0]=data_sck;
    dd[1]=cli_data;
    printf("Data connection established\n");
}

void active_connection(char* address,char* port)
{

}
int power(int a, int b)
{
  int temp=1;
  int i;
for(i=0;i<b;i++){
temp*=2;
}
return temp;
}

int num_to_port(int a,int b)
{
int c,k;
for(c=8;c>=0;c--)
{
k=a>>c;
if(k&1){;b+=power(2,c+8);}
}
return b;
}

char* port_to_num(int a)
{

  
}

void addr(char* addr)
{
int i;
int count=0;
char addres[256];
int p1;
char temp[50];
char* add;
char* port;
char* port2;
strncpy(addres,addr,strlen(addr));
printf("%s %d\n",addr,strlen(addr));
for(i=0;i<strlen(addres);i++){

if(addres[i]==','){count++;}
if(count<4 && addres[i]==','){addres[i]='.';}
}
printf("%s\n",addres);
add=strtok(addres,",");
port=strtok(NULL,",");
port2=strtok(NULL,",");	
printf("%s %s %s\n",add,port,port2);
port_calc(atoi(port),atoi(port2));
}

int respond(void* arg)
{
char** temp;
char buffer[256]=" ";
char* command;
char* atribut;
int status=0;
read((int) arg,buffer,256);
printf("%s",buffer);
command=strtok(buffer," ");
atribut=strtok(NULL," ");

if(strcmp(command,"USER")!=0 && strcmp(command,"PORT")!=0 && strcmp(command,"MKD")!=0 && strcmp(command,"TYPE")!=0 )
{
command[strlen(command)-2]='\0';printf("%s",command);
}

if(strcmp(command,"USER")==0)
{
write((int) arg,r_230,strlen(r_230));printf("%s Response 230\n",command);}

else if(strcmp(command,"MKD")==0)
{
atribut[strlen(atribut)-2]='\0';create_dir(atribut);write((int) arg,r_257,strlen(r_257));printf("%s %s Response 257\n",command,atribut);}

else if(strcmp(command,"PORT")==0)
{
atribut[strlen(atribut)-2]='\0';addr(atribut);write((int) arg,r_200,strlen(r_200));printf("%s Response 200\n",command);}

else if(strcmp(command,"SYST")==0)
{
write((int) arg,r_215,strlen(r_215));printf(" Respons 215\n");}

else if(strcmp(command,"PWD")==0)
{
write((int) arg,r_257,strlen(r_257));printf(" Respons 257\n");}

else if(strcmp(command,"PASV")==0)
{
write((int) arg,r_227,strlen(r_227));printf(" Respons 227\n");passive_connection();}

else if(strcmp(command,"QUIT")==0)
{
status=1;printf(" Respons quit\n");}

else if(strcmp(command,"LIST")==0)
{
write((int) arg,r_150,strlen(r_150));printf(" Respons 150\n");ls(dd[1]);
write((int)arg,r_226,strlen(r_226));close(dd[0]);close(dd[1]);}

else if(strcmp(command,"TYPE")==0)
{
write((int) arg,r_200,strlen(r_200)),printf("%s Respons 200\n",command);}

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
  passive_p=1024;
  passive_port();
  passive_port();
  addr("12,352,333,21,4,21");
  signal(SIGINT,capture);
if(pthread_create (&main_thread,NULL,main_loop,NULL) !=0){
   printf("Thread creation error\n");
   exit(EXIT_FAILURE);
  }
printf("Server runing...\n");
printf("Press <ENTER> to terminate\n");
getc(stdin);
exit(EXIT_SUCCESS);
}
