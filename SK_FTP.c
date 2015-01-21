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

char* r_220="220 Service ready for new user.\n";
char* r_215="215 Unix system type.\n";
char* r_250="250 Requested file action okay, completed.\r\n";
char* r_331="331 User name okay, need password.\n";
char* r_230="230 User logged in, proceed.\n";
char* r_200="200 Command okay.\n";
char* r_500="500 Syntax error, command unrecognized.\n";
char* r_502="502 Command not implemented.\n";
char* r_150="150 File status okay; about to open data connection.\n";
char* r_125="125 Data connection already open; transfer starting.\n";
char* r_226="226 Closing data connection.\n";
char *protocol = "tcp";
char* server_addres="127.0.0.1";

ushort passive_p;
ushort service_port = 21;

pthread_mutex_t lock,lock2;
pthread_t main_thread;

//#################################################
// Przesylanie danych
//#################################################

void send_file(char* file_name,int arg,char* mode,char current_path[256])
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
exit(EXIT_FAILURE);
}
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
      printf("%s",recvBuff);
        write(arg,recvBuff,strlen(recvBuff));
    }
}
close((int)fp);



}

void recive_file(char* file_name,int arg,char* mode,char current_path[256])
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
exit(EXIT_FAILURE);
}
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
close((int)fp);

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

int addr(char* addr,char* result)
{
int i;
int port_r;
int count=0;
char addres[256];
char* port;
char* port2;
strncpy(addres,addr,strlen(addr));
for(i=0;i<strlen(addres);i++){
if(addres[i]==','){count++;}
if(count<4 && addres[i]==','){addres[i]='.';}
}
result=strtok(addres,",");
port=strtok(NULL,",");
port2=strtok(NULL,",");
port_r=num_to_port(atoi(port),atoi(port2));
return port_r;
}

static void capture(int signo){
printf("Signal captured\n");
exit(EXIT_SUCCESS);
}

ushort passive_port()
{
passive_p++;
return passive_p;
}

//#########################################################
//Funkcje serwera
//#########################################################

void ls(int arg,char current_path[256])
{
char cmd[256];
sprintf(cmd,"ls -l %s >> res.txt",current_path);
system(cmd);
FILE *fp;
char wiersz[256];
fp = fopen("res.txt", "rt");
if(NULL == fp)
{
printf("Error opening file");
exit(EXIT_FAILURE);
}
while (fgets(wiersz, 256, fp) != NULL)
{
write(arg,wiersz,strlen(wiersz));
}
system("rm res.txt");
fclose(fp);
}

//############################################################
//Nawiazywanie lacza danych
//############################################################

void passive_connection(int port,int dd[2])
{
struct sockaddr_in serv_data,client_data;
int data_sck,cli_data,cli_len;
bzero(&serv_data, sizeof serv_data);
serv_data.sin_addr.s_addr = INADDR_ANY;
serv_data.sin_family= AF_INET;
serv_data.sin_port = htons(port);
if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
perror("Cannot open socket");
exit(EXIT_FAILURE);
}
if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0){
printf("Cannot bind socket %d to %d port\n",data_sck,port);
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

void active_connection(char* address,ushort port,int dd[2])
{
  int data_sck;
struct sockaddr_in serv_data,client_data;
  int serv_port=passive_port();
bzero(&serv_data, sizeof serv_data);
serv_data.sin_addr.s_addr = INADDR_ANY;
serv_data.sin_family= AF_INET;
serv_data.sin_port = htons(serv_port);

bzero(&client_data, sizeof client_data);
client_data.sin_addr.s_addr = inet_addr("127.0.0.1");
client_data.sin_family= AF_INET;
client_data.sin_port = htons(port);

if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0){
perror("Cannot open socket");
exit(EXIT_FAILURE);
}
dd[0]=data_sck;
if (bind(data_sck,(struct sockaddr*)&serv_data, sizeof serv_data) <0){
printf("Cannot bind socket %d to %d port\n",data_sck,serv_port);
exit(EXIT_FAILURE);}

 if(connect(data_sck, (struct sockaddr *)&client_data, sizeof(client_data))<0)
    {
        printf("\n Error : Connect Failed \n");
        exit(EXIT_FAILURE);
    }
dd[1]=data_sck;
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
int status=0;
while(status==0)
{
memset(buffer,0,256);
command=NULL;
atribut=NULL;
read((int) arg,buffer,256);
printf("%s",buffer);
command=strtok(buffer," ");
atribut=strtok(NULL," ");
if(atribut==NULL)
{
command[strlen(command)-2]='\0';printf("%s",command);
}
if(strcmp(command,"USER")==0)
{
write((int) arg,r_230,strlen(r_230));printf("%s Response 230\n",command);}
else if(strcmp(command,"CWD")==0)
{
atribut[strlen(atribut)-2]='\0';
if(strcmp(atribut,"..")==0)
{
int i;
for(i=strlen(current_path);i>=0;i--)
{
if(current_path[i]=='/'){current_path[i]='\0';break;}
}
}
else {sprintf(current_path,"%s/%s",current_path,atribut);}
write((int) arg,r_200,strlen(r_200));printf("%s Response 200\n",command);}

else if(strcmp(command,"RMD")==0)
{
atribut[strlen(atribut)-2]='\0';
char cmd[256];
sprintf(cmd,"rm -R %s/%s",current_path,atribut);
system(cmd);
write((int)arg,r_250,strlen(r_250));
printf("%s %s Response 250",command,atribut);
}

else if(strcmp(command,"STOR")==0)
{
atribut[strlen(atribut)-2]='\0';
write((int) arg,r_150,strlen(r_150));
recive_file(atribut,desc[1],mode_w,current_path);
write((int) arg,r_226,strlen(r_226));
close(desc[1]);
close(desc[0]);
printf("%s Response 226\n",command);
}
else if(strcmp(command,"RETR")==0)
{
pthread_mutex_lock(&lock2);
atribut[strlen(atribut)-2]='\0';
write((int) arg,r_150,strlen(r_150));
send_file(atribut,desc[1],mode,current_path);
write((int) arg,r_226,strlen(r_226));
close(desc[1]);
close(desc[0]);
printf("%s Response 226\n",command);
pthread_mutex_unlock(&lock2);
}
else if(strcmp(command,"MKD")==0)
{
atribut[strlen(atribut)-2]='\0';
char cmd[50];
sprintf(cmd,"mkdir %s/%s",current_path,atribut);
char r_257[256];
system(cmd);
sprintf(r_257,"257 %s/%s created.\n",current_path,atribut);
write((int) arg,r_257,strlen(r_257));
printf("%s %s Response 257\n",command,atribut);}

else if(strcmp(command,"PORT")==0)
{
char* address="";
int port=0;
atribut[strlen(atribut)-2]='\0';port=addr(atribut,address);active_connection(address,port,desc);write((int) arg,r_200,strlen(r_200));printf("%s Response 200\n",command);}

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
write((int) arg,r_227,strlen(r_227));printf(" Respons 227\n");passive_connection(port,desc);}

else if(strcmp(command,"QUIT")==0)
{
status=1;printf(" Respons quit\n");}

else if(strcmp(command,"LIST")==0)
{
pthread_mutex_lock(&lock);
write((int) arg,r_150,strlen(r_150));printf(" Respons 150\n");ls(desc[1],current_path);
write((int)arg,r_226,strlen(r_226));close(desc[0]);close(desc[1]);
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

void* function(void* arg)
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
passive_p=1045;
if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
    }
if (pthread_mutex_init(&lock2, NULL) != 0)
    {
        printf("\n mutex init failed\n");
    }
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
