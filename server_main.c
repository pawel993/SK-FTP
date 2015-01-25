#include "server_main.h"
#include "responds.h"
#include "transfer.h"
#include "helper_functions.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#define QSIZE 10

static ushort service_port = 21;
char server_addres[50];
pthread_t main_thread;
pthread_mutex_t lock,lock2, lock3;

int running_connections=0;

void ls(int control,int arg,char current_path[],char* mode)
{
  char cmd[256];
  FILE *fp;
  char recvBuff[256];
  int bytesReceived;
  
  (void)snprintf(cmd, 255,"ls -l %s >> res.txt",current_path);
  (void)system(cmd);
  
  fp = fopen("res.txt", mode);
  if(NULL == fp)
  {
    printf("Error opening file");
    (void)write(control,r_550,strlen(r_550));
  }
  else
  {
    (void)write(control,r_150,strlen(r_150));
    if(strcmp(mode,"rb+")==0)
    {
      while((bytesReceived = (int)fread(recvBuff,1,256,fp))>0)
      {
        (void)write(arg,recvBuff,(size_t)bytesReceived);
      }
    }
    else
    {
      while(fgets(recvBuff, 256,fp)!=NULL)
      {
        (void)write(arg,recvBuff,strlen(recvBuff));
      }
    }
  (void)fclose(fp);
  (void)system("rm res.txt");
  (void)write(control,r_226,strlen(r_226));
  }
}

void passive_connection(int control,int port,int dd[])
{
  int error=0;
  struct sockaddr_in serv_data,client_data;
  int data_sck,cli_data,cli_len;
  bzero(&serv_data, sizeof serv_data);
  serv_data.sin_addr.s_addr = INADDR_ANY;
  serv_data.sin_family= AF_INET;
  serv_data.sin_port = htons((uint16_t)port);
  if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
  {
    perror("Cannot open socket");
    error=1;
    goto error_passive;
  }
  if (bind(data_sck,(struct sockaddr*)&serv_data, (socklen_t)sizeof serv_data) <0)
  {
    printf("Cannot bind socket %d to %d port\n",data_sck,port);
    error=1;
    goto error_passive;
  }
  if (listen (data_sck,QSIZE)<0){
    perror("Cannot listen");
    error=1;
    goto error_passive;
  }
  
  printf("Waiting...\n");
  cli_len = (int)sizeof (struct sockaddr_in);
  if ((cli_data = accept(data_sck,(struct sockaddr*) &client_data, (socklen_t*) &cli_len)) < 0)
  {
    perror("Error while connecting with client");
    error=1;
    goto error_passive;
  }
    error_passive:
  if(error==1)
  {
    (void)write(control,r_425,strlen(r_425));
  }
  else
  {
    dd[0]=data_sck;
    dd[1]=cli_data;
    printf("Data connection established\n");
  }
}

void active_connection(int control,char* address,ushort port,int dd[])
{
  int error=0;
  int data_sck;
  struct sockaddr_in serv_data,client_data;
  int serv_port=active_port();
  bzero(&serv_data, sizeof serv_data);
  serv_data.sin_addr.s_addr = INADDR_ANY;
  serv_data.sin_family= AF_INET;
  serv_data.sin_port = htons(serv_port);

  bzero(&client_data, sizeof client_data);
  client_data.sin_addr.s_addr = inet_addr(address);
  client_data.sin_family= AF_INET;
  client_data.sin_port = htons(port);

  if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
  {
    perror("Cannot open socket");
    error=1;
    goto error_active;
  }
  dd[0]=data_sck;
  if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0)
  {
    printf("Cannot bind socket %d to %d port\n",data_sck,serv_port);
    error=1;
    goto error_active;
  }

  if(connect(data_sck, (struct sockaddr *)&client_data, sizeof(client_data))<0)
  {
    printf("\n Error : Connect Failed \n");
    error=1;
    goto error_active;
  }
    error_active:
  if(error==1)
  {
    (void)write(control,r_425,strlen(r_425));
  }
  else
  {
    dd[1]=data_sck;
    (void)write(control,r_200,strlen(r_200));
    printf("Connection established\n");
  }
}

void commandCWD(int arg, char atribut[], char current_path[], char command[])
{
  char temp[256];
  char path[256];
  char temp2[256];
  (void)snprintf(temp, 255, "%s", current_path);
  getcwd(temp2, 256);
  (void)snprintf(path, 255, "%s/Files", temp2);
  atribut[strlen(atribut)-2]='\0';
  if(strcmp(atribut,"..")==0)
  {
    int i;
    for(i=strlen(current_path);i>=0;i--)
    {
      if(current_path[i]=='/') 
      {
        current_path[i]='\0';
        break;  
      }
    }
  }
  else 
  {
   if(atribut[0]!='/')
   {
     char copy[256];
     (void)snprintf(copy, 255, "%s", current_path);
     (void)snprintf(current_path, 255,"%s/%s",copy,atribut);
   }
   else 
     (void)snprintf(current_path, 255, "%s",atribut);
  }
   
  if(strlen(current_path)<strlen(path))
  {
    (void)snprintf(current_path, 255,"%s",temp);
    write(arg,r_550,strlen(r_550));
    printf("%s Response 550\n",command);
    printf("%s\n",current_path);
  }
  else
  {
    write(arg,r_200,strlen(r_200));
    printf("%s Response 200\n",command);
  }
}

void commandRMD(int arg, char atribut[], char current_path[], char command[])
{
  char cmd[256];
  atribut[strlen(atribut)-2]='\0';
  (void)snprintf(cmd, 255,"rmdir %s/%s",current_path,atribut);
  if(atribut[0]!='/')
    (void)snprintf(cmd, 255,"rmdir %s/%s",current_path,atribut);
  else 
    (void)snprintf(cmd, 255,"rmdir %s",atribut);
    system(cmd);
    write(arg,r_250,strlen(r_250));
    printf("%s %s Response 250",command,atribut);
}

void commandMKD(int arg, char atribut[], char current_path[], char command[])
{
  char cmd[256];
  char r_257[256];
  atribut[strlen(atribut)-2]='\0';
  if(atribut[0]!='/')
    (void)snprintf(cmd, 255,"mkdir %s/%s",current_path,atribut);
  else 
    (void)snprintf(cmd, 255,"mkdir %s",atribut);
  system(cmd);
  (void)snprintf(r_257, 255,"257 %s/%s created.\n",current_path,atribut);
  write(arg,r_257,strlen(r_257));
  printf("%s %s Response 257\n",command,atribut);
}

void commandPORT(int arg, char atribut[], char current_path[], char command[], int desc[])
{
  char* address;
  int i;
  int port_r;
  int count=0;
  char addres[256];
  char* port;
  char* port2;
  char* save_part;
  atribut[strlen(atribut)-2]='\0';
  strncpy(addres,atribut,strlen(atribut));
  for(i=0;i<strlen(addres);i++)
  {
    if(addres[i]==',')
      count++;
    if(count<4 && addres[i]==',')
      addres[i]='.';
   }
   
   address=strtok_r(addres,",",&save_part);
   port=strtok_r(NULL,",",&save_part);
   port2=strtok_r(NULL,",",&save_part);
   port_r=num_to_port(atoi(port),atoi(port2));
   active_connection(arg,address,port_r,desc);
   printf("%s Response 200\n",command);
}

void respond(int arg,char current_path[])
{
  int desc[2];
  char* mode="rb+";
  char* mode_w="wb+";
  char buffer[256]="";
  char* command;
  char* atribut;
  char* save_part;
  int status=0;
  while(status==0)  
  {
    memset(buffer,0,256);
    command=NULL;
    atribut=NULL;
    save_part=NULL;
    read(arg,buffer,256);
    printf("%s",buffer);
    command=strtok_r(buffer," ",&save_part);
    atribut=strtok_r(NULL," ",&save_part);
    if(atribut==NULL)
    {
      command[strlen(command)-2]='\0';
      printf("%s",command);
    }
    
    if(strcmp(command,"USER")==0)
    {
      write(arg,r_230,strlen(r_230));
      printf("%s Response 230\n",command);
    }
    
    else if(strcmp(command,"CWD")==0)
    {
      commandCWD(arg, atribut, current_path, command);
    }

    else if(strcmp(command,"RMD")==0)
    {
      commandRMD(arg, atribut, current_path, command);
    }

    else if(strcmp(command,"STOR")==0)
    {
      atribut[strlen(atribut)-2]='\0';
      recive_file(arg,atribut,desc[1],mode_w,current_path);
      close(desc[1]);
      close(desc[0]);
      printf("%s Response 226\n",command);
    }
    
    else if(strcmp(command,"RETR")==0)
    {
      pthread_mutex_lock(&lock2);
      atribut[strlen(atribut)-2]='\0';
      send_file(arg,atribut,desc[1],mode,current_path);
      close(desc[1]);
      close(desc[0]);
      printf("%s Response 226\n",command);
      pthread_mutex_unlock(&lock2);
    }
    
    else if(strcmp(command,"MKD")==0)
    {
        commandMKD(arg, atribut, current_path, command);
    }

    else if(strcmp(command,"PORT")==0)
    {
      commandPORT(arg, atribut, current_path, command, desc);
    }

    else if(strcmp(command,"SYST")==0)
    {
      write(arg,r_215,strlen(r_215));
      printf(" Respons 215\n");
    }

    else if(strcmp(command,"PWD")==0)
    {       
      char r_257[256];
      (void)snprintf(r_257, 255,"257 %s current working directory.\n",current_path);
      write(arg,r_257,strlen(r_257));printf(" Respons 257\n");
    }

    else if(strcmp(command,"PASV")==0)
    {
      int port=passive_port();
      char ad[256];
      char r_227[256];
      port_to_num(server_addres,port,ad);   
      
      (void)snprintf(r_227, 255,"227 Entering Passive Mode %s",ad);
      write(arg,r_227,strlen(r_227));
      printf(" Respons 227\n");
      passive_connection(arg,port,desc);
    }

    else if(strcmp(command,"QUIT")==0)
    {
      status=1;
      printf(" Respons quit\n");
    }

    else if(strcmp(command,"LIST")==0)
    {
      pthread_mutex_lock(&lock);
      printf(" Respons 150\n");
      ls(arg,desc[1],current_path,mode);
      close(desc[0]);
      close(desc[1]);
      pthread_mutex_unlock(&lock);
    }

    else if(strcmp(command,"TYPE")==0)
    {
      atribut[strlen(atribut)-2]='\0';
      if(strcmp(atribut,"A")==0)
      {
        mode="rt+";
        mode_w="wt+";
      }
      else 
      {
        mode="rb+";
        mode_w="wb+";
      }
      write(arg,r_200,strlen(r_200));
      printf("%s Respons 200\n",command);
    }

    else 
    {
      write(arg,r_500,strlen(r_500));
      printf(" Response 500\n");
    }
  }
}

void* control_connection(void* arg)
{
  char current_path[256];
  char temp[256];
  int sck = *(int*) arg;
  running_connections++;
  pthread_mutex_unlock(&lock3);
  printf("Connection established..\n");
  (void)write(sck,r_220,strlen(r_220));
  (void)getcwd(temp,256);
  (void)snprintf(current_path, 255,"%s/Files",temp);
  respond(sck,current_path);
  
  close(sck);
  pthread_mutex_lock(&lock3);
  running_connections--;
  pthread_mutex_unlock(&lock3);
  
  pthread_detach(pthread_self());
  return 0;
}

void* main_loop(/*@unused@*/void* arg)
{
  struct sockaddr_in server_addr,client_addr;
  int sck,rcv_sck,rcv_len;
  
  bzero(&server_addr, sizeof server_addr);
  
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_family= AF_INET;
  server_addr.sin_port = htons(service_port);
  
  if ((sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
  {
    perror("Cannot open socket");
    exit(EXIT_FAILURE);
  }
  
  if (bind(sck,(struct sockaddr*)&server_addr, (socklen_t)sizeof server_addr) <0)
  {
    printf("Cannot bind socket %d to %d port\n",(int)sck, (int)service_port);
    exit(EXIT_FAILURE);
  }
  
  if (listen (sck,QSIZE)<0)
  {
    perror("Cannot listen");
    exit(EXIT_FAILURE);
  }
  pthread_mutex_init(&lock3, NULL);
  while(1==1)
  {
    rcv_len = (int)sizeof (struct sockaddr_in);
    if ((rcv_sck = accept(sck,(struct sockaddr*) &client_addr, (socklen_t*) &rcv_len)) < 0)
    {
      perror("Error while connecting with client");
    }
    else
    {
      if(running_connections > 19)
      {
        close(rcv_sck);
        continue;
      }
      pthread_mutex_lock(&lock3);
      (void)snprintf(server_addres,40,"%s",inet_ntoa(client_addr.sin_addr));
      if(pthread_create (&main_thread,NULL,control_connection,(void*) &rcv_sck) !=0)
      {
        printf("Thread creation error\n");
        close(rcv_sck);
        close(sck);
        exit(EXIT_FAILURE);
      }
    }
  }
  
  (void)close(sck);
  return (void*) 0;
}