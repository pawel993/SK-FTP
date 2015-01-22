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
#include <arpa/inet.h>
#define QSIZE 10
#define BUFSIZE 10000

char* r_220="220 Service ready for new user.\n";
char* r_215="215 Unix system type.\n";
char* r_250="250 Requested file action okay, completed.\r\n";
char* r_331="331 User name okay, need password.\n";
char* r_230="230 User logged in, proceed.\n";
char* r_200="200 Command okay.\n";
char* r_500="500 Syntax error, command unrecognized.\n";
char* r_502="502 Command not implemented.\n";
char* r_150="150 File status okay; about to open data connection.\n";
char* r_226="226 Closing data connection.\n";
char* r_550="550 Requested action not taken.File unavailable.\n";
char* r_425="425 Can't open data connection.\n";
char *protocol = "tcp";
char* server_addres="127.0.0.1";

ushort passive_p;
ushort active_p;
ushort service_port = 21;

pthread_mutex_t lock,lock2;
pthread_t main_thread;

//#################################################
// Przesylanie danych
//#################################################

void send_file(void* control,char* file_name,int arg,char* mode,char current_path[256])
{
int bytesReceived;
char recvBuff[256];
char file[256];
sprintf(file,"%s/%s",current_path,file_name);
FILE *fp;
fp = fopen(file, mode);
if(NULL == fp)
{
printf("Error opening file");
write((int) control,r_550,strlen(r_550));
}
else
{
write((int) control,r_150,strlen(r_150));
if(strcmp(mode,"rb+")==0)
{

 while((bytesReceived = fread(recvBuff,1,256,fp))>0)
    {
        write(arg,recvBuff,bytesReceived);
    }
}
else
{
  while(fgets(recvBuff, 256,fp)!=NULL)
    {
        write(arg,recvBuff,strlen(recvBuff));
    }
}
close((int)fp);
write((int) control,r_226,strlen(r_226));
}


}

void recive_file(void* control,char* file_name,int arg,char* mode,char current_path[256])
{
int bytesReceived;
char recvBuff[256];
char file[256];
if(file_name[0]!='/')sprintf(file,"%s/%s",current_path,file_name);
else sprintf(file,"%s",file_name);
FILE *fp;
fp = fopen(file, mode);
if(NULL == fp)
{
printf("Error opening file");
write((int) control,r_550,strlen(r_550));
}
else{
write((int) control,r_150,strlen(r_150));
if(strcmp(mode,"wb+")==0)
{
 printf("Binary mode\n");
 while((bytesReceived = read(arg, recvBuff, 256)) > 0)
    {
        fwrite(recvBuff,1,bytesReceived,fp);
    }
}
else
{
  while((bytesReceived = read(arg, recvBuff, 256)) > 0)
    {
        fputs(recvBuff,fp);
    }
}
write((int) control,r_226,strlen(r_226));
fflush(fp);
close((int)fp);
}
}


//##########################################################
//Funkcje pomocnicze
//##########################################################

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
void port_to_num(char* server_addres,int a,char result[256])
{
char temp[60];
int c,k;
int r1=0,r2=0;
for(c=16;c>=0;c--)
{
  k=a>>c;
  if(k&1 && c>8){r1+=power(2,c-8);}
  else if(k&1) {r2+=power(2,c);}
} 
strncpy(temp,server_addres,strlen(server_addres));
for(c=0;c<strlen(server_addres);c++){
if(temp[c]=='.')temp[c]=',';
}
sprintf(result,"(%s,%d,%d).\n",temp,r1,r2);
}

ushort active_port()
{
active_p++;
return active_p;
}

ushort passive_port()
{
passive_p++;
return passive_p;
}

//#########################################################
//Funkcje serwera
//#########################################################

void ls(void* control,int arg,char current_path[256],char* mode)
{
char cmd[256];
sprintf(cmd,"ls -l %s >> res.txt",current_path);
system(cmd);
FILE *fp;
char recvBuff[256];
int bytesReceived;
fp = fopen("res.txt", mode);
if(NULL == fp)
{
printf("Error opening file");
write((int) control,r_550,strlen(r_550));
}
else{
write((int) control,r_150,strlen(r_150));
if(strcmp(mode,"rb+")==0)
{
 while((bytesReceived = fread(recvBuff,1,256,fp))>0)
    {
        write(arg,recvBuff,bytesReceived);
    }
}
else
{
  while(fgets(recvBuff, 256,fp)!=NULL)
    {
        write(arg,recvBuff,strlen(recvBuff));
    }
}
system("rm res.txt");
fclose(fp);
write((int)control,r_226,strlen(r_226));
}
}

//############################################################
//Nawiazywanie lacza danych
//############################################################

void passive_connection(void* control,int port,int dd[2])
{
int error=0;
struct sockaddr_in serv_data,client_data;
int data_sck,cli_data,cli_len;
bzero(&serv_data, sizeof serv_data);
serv_data.sin_addr.s_addr = INADDR_ANY;
serv_data.sin_family= AF_INET;
serv_data.sin_port = htons(port);
if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
perror("Cannot open socket");
error=1;
goto error_passive;
}
if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0){
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
cli_len = sizeof (struct sockaddr_in);
if ((cli_data = accept(data_sck,(struct sockaddr*) &client_data, (socklen_t*) &cli_len)) < 0){
perror("Error while connecting with client");
error=1;
goto error_passive;

}
error_passive:
if(error==1){write((int)control,r_425,strlen(r_425));}
else
{
dd[0]=data_sck;
dd[1]=cli_data;
printf("Data connection established\n");
}
}

void active_connection(void* control,char* address,ushort port,int dd[2])
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

if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
perror("Cannot open socket");
error=1;
goto error_active;
}
dd[0]=data_sck;
if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0){
printf("Cannot bind socket %d to %d port\n",data_sck,serv_port);
error=1;
goto error_active;}

 if(connect(data_sck, (struct sockaddr *)&client_data, sizeof(client_data))<0)
    {
        printf("\n Error : Connect Failed \n");
        error=1;
        goto error_active;
    }
error_active:
if(error==1){write((int)control,r_425,strlen(r_425));}
else{
dd[1]=data_sck;
write((int) control,r_200,strlen(r_200));
printf("Connection established\n");
}
}

//##################################################
//Odpowiadanie na zadania klienta
//##################################################

void respond(void* arg,char current_path[256])
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
read((int) arg,buffer,256);
printf("%s",buffer);
command=strtok_r(buffer," ",&save_part);
atribut=strtok_r(NULL," ",&save_part);
if(atribut==NULL)
{
command[strlen(command)-2]='\0';printf("%s",command);
}
if(strcmp(command,"USER")==0)
{
write((int) arg,r_230,strlen(r_230));printf("%s Response 230\n",command);}
else if(strcmp(command,"CWD")==0)
{
char temp[256];
sprintf(temp,"%s",current_path);
char path[256];
getcwd(path,256);
sprintf(path,"%s/Files",path);
atribut[strlen(atribut)-2]='\0';
if(strcmp(atribut,"..")==0)
{
int i;
for(i=strlen(current_path);i>=0;i--)
{
if(current_path[i]=='/'){current_path[i]='\0';break;}
}
}
else {if(atribut[0]!='/')sprintf(current_path,"%s/%s",current_path,atribut);
else sprintf(current_path,"%s",atribut);
}
if(strlen(current_path)<strlen(path))
  {current_path=temp;
  write((int) arg,r_550,strlen(r_550));printf("%s Response 550\n",command);printf("%s\n",current_path);}
else
{
write((int) arg,r_200,strlen(r_200));printf("%s Response 200\n",command);}
}

else if(strcmp(command,"RMD")==0)
{
atribut[strlen(atribut)-2]='\0';
char cmd[256];
sprintf(cmd,"rmdir %s/%s",current_path,atribut);
if(atribut[0]!='/')sprintf(cmd,"rmdir %s/%s",current_path,atribut);
else sprintf(cmd,"rmdir %s",atribut);
system(cmd);
write((int)arg,r_250,strlen(r_250));
printf("%s %s Response 250",command,atribut);
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
atribut[strlen(atribut)-2]='\0';
char cmd[50];
if(atribut[0]!='/')sprintf(cmd,"mkdir %s/%s",current_path,atribut);
else sprintf(cmd,"mkdir %s",atribut);
char r_257[256];
system(cmd);
sprintf(r_257,"257 %s/%s created.\n",current_path,atribut);
write((int) arg,r_257,strlen(r_257));
printf("%s %s Response 257\n",command,atribut);}

else if(strcmp(command,"PORT")==0)
{
char* address;
atribut[strlen(atribut)-2]='\0';
int i;
int port_r;
int count=0;
char addres[256];
char* port;
char* port2;
strncpy(addres,atribut,strlen(atribut));
for(i=0;i<strlen(addres);i++)
{
if(addres[i]==','){count++;}
if(count<4 && addres[i]==','){addres[i]='.';}
}
char* save_part;
address=strtok_r(addres,",",&save_part);
port=strtok_r(NULL,",",&save_part);
port2=strtok_r(NULL,",",&save_part);
port_r=num_to_port(atoi(port),atoi(port2));
active_connection(arg,address,port_r,desc);printf("%s Response 200\n",command);}

else if(strcmp(command,"SYST")==0)
{
write((int) arg,r_215,strlen(r_215));printf(" Respons 215\n");}

else if(strcmp(command,"PWD")==0)
{
char r_257[256];
sprintf(r_257,"257 %s current working directory.\n",current_path);
write((int) arg,r_257,strlen(r_257));printf(" Respons 257\n");}

else if(strcmp(command,"PASV")==0)
{
int port=passive_port();
char ad[256];
port_to_num(server_addres,port,ad);
char r_227[256];
sprintf(r_227,"227 Entering Passive Mode %s",ad);
write((int) arg,r_227,strlen(r_227));printf(" Respons 227\n");
passive_connection(arg,port,desc);}

else if(strcmp(command,"QUIT")==0)
{
status=1;printf(" Respons quit\n");}

else if(strcmp(command,"LIST")==0)
{
pthread_mutex_lock(&lock);
printf(" Respons 150\n");ls(arg,desc[1],current_path,mode);
close(desc[0]);close(desc[1]);
pthread_mutex_unlock(&lock);
}

else if(strcmp(command,"TYPE")==0)
{
atribut[strlen(atribut)-2]='\0';
if(strcmp(atribut,"A")==0){mode="rt+";mode_w="wt+";}
else {mode="rb+";mode_w="wb+";}
write((int) arg,r_200,strlen(r_200)),printf("%s Respons 200\n",command);}

else {write((int) arg,r_500,strlen(r_500));printf(" Response 500\n");}
  
}
}
//#########################################
// Tworzenie watkow i przyjmowanie polaczen
//#########################################

void* control_connection(void* arg)
{
printf("Connection established..\n");
write((int) arg,r_220,strlen(r_220));
char current_path[256];
getcwd(current_path,256);
sprintf(current_path,"%s/Files",current_path);
respond(arg,current_path);
close((int) arg);
return 0;
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
}
else
{
if(pthread_create (&main_thread,NULL,control_connection,(void*) rcv_sck) !=0){
printf("Thread creation error\n");
exit(EXIT_FAILURE);
}
}
}
close(sck);
}
int main(int argc,char *argv[])
{
system("mkdir Files");
passive_p=1045;
active_p=2024;
pthread_mutex_init(&lock, NULL);
pthread_mutex_init(&lock2, NULL);
if(pthread_create (&main_thread,NULL,main_loop,NULL) !=0){
printf("Thread creation error\n");
exit(EXIT_FAILURE);
}
printf("Server runing...\n");
printf("Press <ENTER> to terminate\n");
getc(stdin);
exit(EXIT_SUCCESS);
}
