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

static ushort service_port = 21;//port połaczenia kontrolnego
char server_addres[50];//adres serwera
pthread_t main_thread;
pthread_mutex_t lock,lock2, lock3;

//liczba aktywnych połączeń
int running_connections=0;

/*
 * realizacja komendy ls
 * control - identyfikator połączenia kontrolnego
 * arg - identyfikator połączenia danych
 * current_path -  aktualna ścieżka
 * mode - aktualny tryb dostępu do pliku
 */
void ls(int control,int arg,char current_path[],char* mode)
{
  char cmd[256];
  FILE *fp;
  char recvBuff[256];
  int bytesReceived;
  
  //wywołanie komendy ls i zapisanie wyniku do pliku pomocniczego
  (void)snprintf(cmd, 255,"ls -l %s >> res.txt",current_path);
  (void)system(cmd);
  
  //próba otworzenia pliku
  fp = fopen("res.txt", mode);
  if(NULL == fp)
  {
    printf("Error opening file");
    (void)write(control,r_550,strlen(r_550));
  }
  else
  {
    //przesłanie łączem danych wyniku polecenia ls
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
  //zamknięcie i usunięcie pliku
  (void)fclose(fp);
  (void)system("rm res.txt");
  (void)write(control,r_226,strlen(r_226));
  }
}

/*
 * Nawiązanie pasywne połączenia z klientem
 * control - identyfikator łącza kontrolnego
 * port - numer portu
 * dd - tablica, wktórą wpisane zostają numery portów
 */
void passive_connection(int control,int port,int dd[])
{
  int error=0;
  struct sockaddr_in serv_data,client_data;
  int data_sck,cli_data,cli_len;
  bzero(&serv_data, sizeof serv_data);
  serv_data.sin_addr.s_addr = INADDR_ANY;
  serv_data.sin_family= AF_INET;
  serv_data.sin_port = htons((uint16_t)port);
  
  //próba otworzenia socketu
  if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
  {
    perror("Cannot open socket");
    error=1;
    goto error_passive;
  }
  
  //próba "zbindowania"(sic?!)
  if (bind(data_sck,(struct sockaddr*)&serv_data, (socklen_t)sizeof serv_data) <0)
  {
    printf("Cannot bind socket %d to %d port\n",data_sck,port);
    error=1;
    goto error_passive;
  }
  
  //próba nasłuchu
  if (listen (data_sck,QSIZE)<0){
    perror("Cannot listen");
    error=1;
    goto error_passive;
  }
  
  printf("Waiting...\n");
  cli_len = (int)sizeof (struct sockaddr_in);
  
  //próba nawiązania połączenia z klientem
  if ((cli_data = accept(data_sck,(struct sockaddr*) &client_data, (socklen_t*) &cli_len)) < 0)
  {
    perror("Error while connecting with client");
    error=1;
    goto error_passive;
  }
  
  //obsługa błędu nazwiązania połączenia
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

/*
 * Nawiązanie aktywne połączenia z klientem
 * control - identyfikator łącza kontrolnego
 * address - adres serwera
 * port - numer portu
 * dd - tablica, w którą wpisane zostają numery portów
 * 
 */
void active_connection(int control,char* address,ushort port,/*@out@*/int dd[])
{
  int error=0;
  int data_sck;
  
  struct sockaddr_in serv_data,client_data;
  int serv_port=(int)active_port();
  
  bzero(&serv_data, sizeof serv_data);
  serv_data.sin_addr.s_addr = INADDR_ANY;
  serv_data.sin_family= AF_INET;
  serv_data.sin_port = htons((uint16_t)serv_port);

  bzero(&client_data, sizeof client_data);
  client_data.sin_addr.s_addr = inet_addr(address);
  client_data.sin_family= AF_INET;
  client_data.sin_port = htons((uint16_t)port);

  //próba otworzenia socketu
  if ((data_sck = socket (PF_INET,SOCK_STREAM,IPPROTO_TCP))<0)
  {
    perror("Cannot open socket");
    error=1;
    goto error_active;
  }
  dd[0]=data_sck;
  
  if (bind(data_sck,(struct sockaddr*)&serv_data, (socklen_t)sizeof serv_data) <0)
  {
    printf("Cannot bind socket %d to %d port\n",data_sck,serv_port);
    error=1;
    goto error_active;
  }
  
  //próba nazwiązania połaczenia
  if(connect(data_sck, (struct sockaddr *)&client_data, (socklen_t)sizeof(client_data))<0)
  {
    printf("\n Error : Connect Failed \n");
    error=1;
    goto error_active;
  }
  
  //obsługa błedów nawiązywania połaczenia
    error_active:
  if(error==1)
  {
    (void)write(control,r_425,strlen(r_425));
  }
  else
  {
    //komunikat o nawiązaniu połączenia
    dd[1]=data_sck;
    (void)write(control,r_200,strlen(r_200));
    printf("Connection established\n");
  }
}


/*
 * Wykonianie komendy CWD
 * arg - numer łącza kontrolnego
 * atribut - atrybut
 * current_path - aktualna ścieżka
 * command - komenda otrzymana od klienta
 */
void commandCWD(int arg,/*@null@*/ char atribut[], char current_path[], char command[])
{
  char temp[256];
  char path[256];
  char temp2[256];
  
  //wyganerowanie aktualnych ścieżek i wpisanie aktualnej ścieżki danego klienta
  (void)snprintf(temp, 255, "%s", current_path);
  (void)getcwd(temp2, 256);
  (void)snprintf(path, 255, "%s/Files", temp2);
  atribut[strlen(atribut)-2]='\0';
  
  //sprawdzenie, czy nie jest to komenda wycofania do nadrzędnego katalogu
  if(strcmp(atribut,"..")==0)
  {
    //skrócenie aktualnej ścieżki o ostatni folder
    int i;
    for(i=(int)strlen(current_path);i>=0;i--)
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
    //jeżeli to komenda przejścia do podrzędnego katalogu
    //
    
   if(atribut[0]!='/')
   {
     //jeżeli scieżka ma format względny
     char copy[256];
     (void)snprintf(copy, 255, "%s", current_path);
     (void)snprintf(current_path, 255,"%s/%s",copy,atribut);
   }
   //jeżeli ścieżka absolutna
   else 
     (void)snprintf(current_path, 255, "%s",atribut);
  }
   
  //test, czy klient nie próbuje wyjść poza dostępną przestrzeń katalogów
  //do katalogu nadrzędnego względem Files w aktualnym katalogu roboczym sewera
  if(strlen(current_path)<strlen(path))
  {
    (void)snprintf(current_path, 255,"%s",temp);
    (void)write(arg,r_550,strlen(r_550));
    printf("%s Response 550\n",command);
    printf("%s\n",current_path);
  }
  else
  {
    (void)write(arg,r_200,strlen(r_200));
    printf("%s Response 200\n",command);
  }
}

/*
 * Wykonanie komendy RMD
 * arg - identyfikator łącza kontrolnego
 * atribut - atrybut komendy
 * current_path - aktualny katalog roboczy połaczenia
 * command - komenda klienta
 */
void commandRMD(int arg, /*@null@*/char atribut[], char current_path[], char command[])
{
  char cmd[256];
  atribut[strlen(atribut)-2]='\0';
  
  //przygotowanie komendy (ścieżka relatywna/absolutna)
  (void)snprintf(cmd, 255,"rmdir %s/%s",current_path,atribut);
  if(atribut[0]!='/')
    (void)snprintf(cmd, 255,"rmdir %s/%s",current_path,atribut);
  else 
    (void)snprintf(cmd, 255,"rmdir %s",atribut);
  
  //wykonanie komendy
  (void)system(cmd);
  (void)write(arg,r_250,strlen(r_250));
  printf("%s %s Response 250",command,atribut);
}

/*
 * Wykonanie komendy MKD
 * arg - numer łącza kontrolnego
 * atribut - atrybut
 * current_path - aktualna ścieżka
 * command - komenda otrzymana od klienta
 */
void commandMKD(int arg, /*@null@*/char atribut[], char current_path[], char command[])
{
  char cmd[256];
  char r_257[256];
  atribut[strlen(atribut)-2]='\0';
  
  //przygotowanie komendy w zależności od typu ścieżki
  if(atribut[0]!='/')
    (void)snprintf(cmd, 255,"mkdir %s/%s",current_path,atribut);
  else 
    (void)snprintf(cmd, 255,"mkdir %s",atribut);
  (void)system(cmd);
  
  //komenda o powodzeniu
  (void)snprintf(r_257, 255,"257 %s/%s created.\n",current_path,atribut);
  (void)write(arg,r_257,strlen(r_257));
  printf("%s %s Response 257\n",command,atribut);
}

/*
 * Wykonanie komendy PORT
 * 
 * arg - numer łącza kontrolnego
 * atribut - atrybut
 * current_path - aktualna ścieżka
 * command - komenda otrzymana od klienta
 * desc - tablica wyjściowa z numerami portów
 */
void commandPORT(int arg, /*@null@*/ char atribut[], /*@unused@*/char current_path[], char command[], /*@out@*/int desc[])
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
  //zmiena formatu zapisu adresu
  for(i=0;i<(int)strlen(addres);i++)
  {
    if(addres[i]==',')
      count++;
    if(count<4 && addres[i]==',')
      addres[i]='.';
   }
   
   //podział adresu na części składowe
   address=strtok_r(addres,",",&save_part);
   port=strtok_r(NULL,",",&save_part);
   port2=strtok_r(NULL,",",&save_part);
   port_r=num_to_port(atoi(port),atoi(port2));
   
   //aktywne nawiązanie połaczenia
   active_connection(arg,address,(ushort)port_r,desc);
   printf("%s Response 200\n",command);
}


/*
 * Analiza komend i ich wykonywanie
 * arg - identyfikator połączenia kontrolnego
 * current_path - aktualna ścieżka
 */
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
  
  //pętla wykonywana aż do otrzymania komendy QUIT (zmiana status na 1)
  while(status==0)  
  {
    memset(buffer,0,256);
    command=NULL;
    atribut=NULL;
    save_part=NULL;
    
    //pobranie komendy
    (void)read(arg,buffer,256);
    printf("%s",buffer);
    
    //rozłożenie komendy na składniki
    command=strtok_r(buffer," ",&save_part);
    atribut=strtok_r(NULL," ",&save_part);
    if(atribut==NULL)
    {
      atribut=&command[0];
      command[strlen(command)-2]='\0';
      printf("%s",command);
    }
    
    //sprawdzenie typu komendy i wykoanie odpowiedniej akcji
    if(strcmp(command,"USER")==0)
    {
      //potwierdzenie odbioru nazwy użytkownika
      (void)write(arg,r_230,strlen(r_230));
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
      //polecenie odebrania pliku
      atribut[strlen(atribut)-2]='\0';
      recive_file(arg,atribut,desc[1],mode_w,current_path);
      (void)close(desc[1]);
      (void)close(desc[0]);
      printf("%s Response 226\n",command);
    }
    
    else if(strcmp(command,"RETR")==0)
    {
      //polecenie wysłania pliku
      (void)pthread_mutex_lock(&lock2);
      atribut[strlen(atribut)-2]='\0';
      send_file(arg,atribut,desc[1],mode,current_path);
      (void)close(desc[1]);
      (void)close(desc[0]);
      printf("%s Response 226\n",command);
      (void)pthread_mutex_unlock(&lock2);
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
      //odpowiedź na zapytanie o system
      (void)write(arg,r_215,strlen(r_215));
      printf(" Respons 215\n");
    }

    else if(strcmp(command,"PWD")==0)
    {       
      //przesłanie aktualnego katalogu roboczego połączenia
      char r_257[256];
      (void)snprintf(r_257, 255,"257 %s current working directory.\n",current_path);
      (void)write(arg,r_257,strlen(r_257));
      printf(" Respons 257\n");
    }

    else if(strcmp(command,"PASV")==0)
    {
      //sekwencja poleceń do nawiązania połączenia pasywnie
      int port=(int)passive_port();
      char ad[256];
      char r_227[256];
      port_to_num(server_addres,port,ad);   
      
      (void)snprintf(r_227, 255,"227 Entering Passive Mode %s",ad);
      (void)write(arg,r_227,strlen(r_227));
      printf(" Respons 227\n");
      passive_connection(arg,port,desc);
    }

    else if(strcmp(command,"QUIT")==0)
    {
      //zakończenie połączenia
      status=1;
      printf(" Respons quit\n");
    }

    else if(strcmp(command,"LIST")==0)
    {
      //wypisanie listy plików z aktualnego katalogu roboczego
      (void)pthread_mutex_lock(&lock);
      printf(" Respons 150\n");
      ls(arg,desc[1],current_path,mode);
      (void)close(desc[0]);
      (void)close(desc[1]);
      (void)pthread_mutex_unlock(&lock);
    }

    else if(strcmp(command,"TYPE")==0)
    {
      //ustawnienie typu transferu danych (binarne/ascii)
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
      (void)write(arg,r_200,strlen(r_200));
      printf("%s Respons 200\n",command);
    }

    else 
    {
      //wysłanie informacji o nieznanej komendzie
      (void)write(arg,r_500,strlen(r_500));
      printf(" Response 500\n");
    }
  }
}

/*
 * ustanowienie połaczenia kontrolnego
 * inicjacja parametrów połączenia z klientem
 * arg = wskaźnik na identyfikator łącza kontrolnego
 */
void* control_connection(void* arg)
{
  char current_path[256];
  char temp[256];
  
  //pobranie identyfikatora łącza kontrolnego
  int sck = *(int*) arg;
  running_connections++;
  (void)pthread_mutex_unlock(&lock3);
  printf("Connection established..\n");
  
  //określenie katalogu roboczego
  (void)write(sck,r_220,strlen(r_220));
  (void)getcwd(temp,256);
  (void)snprintf(current_path, 255,"%s/Files",temp);
  
  //oczekiwanie i wykonywanie instrukcji
  respond(sck,current_path);
  
  //zakończenie połączenia i wątku
  (void)close(sck);
  (void)pthread_mutex_lock(&lock3);
  running_connections--;
  (void)pthread_mutex_unlock(&lock3);
  
  (void)pthread_detach(pthread_self());
  return arg;
}


/*
 * główna pętla serwera
 * reagowanie na przychodzące połaczenia
 * tworzenie wątków obsługujących poszczególnych klientów
 */
void* main_loop(void* arg)
{
  struct sockaddr_in server_addr,client_addr;
  int sck,rcv_sck,rcv_len;
  
  bzero(&server_addr, sizeof server_addr);
  
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_family= AF_INET;
  server_addr.sin_port = htons((uint16_t)service_port);
  
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
  
  //inicjacja semafora chroniącego dane o nowo nawiązanym połączeniu
  (void)pthread_mutex_init(&lock3, NULL);
  while(1==1)
  {
    rcv_len = (int)sizeof (struct sockaddr_in);
    if ((rcv_sck = accept(sck,(struct sockaddr*) &client_addr, (socklen_t*) &rcv_len)) < 0)
    {
      perror("Error while connecting with client");
    }
    else
    {
      //ograniczenie do 20 połączenień, kolejne są odrzucane
      if(running_connections > 19)
      {
        (void)close(rcv_sck);
        continue;
      }
      
      //zamknięcie semafora i stworzenie nowego wątku obsługującego nowego klienta
      (void)pthread_mutex_lock(&lock3);
      (void)snprintf(server_addres,40,"%s",inet_ntoa(client_addr.sin_addr));
      if(pthread_create (&main_thread,NULL,control_connection,(void*) &rcv_sck) !=0)
      {
        printf("Thread creation error\n");
        (void)close(rcv_sck);
        (void)close(sck);
        exit(EXIT_FAILURE);
      }
    }
  }
  
  (void)close(sck);
  return 0;
}