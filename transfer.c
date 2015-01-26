#include "transfer.h"
#include "responds.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>


/*
 * Funkcja wysyłająca plik do klienta
 * control - identyfikator połączenia kontrolnego
 * file_name - nazwa przesyłanego pliku
 * arg - identyfikator połączenia danych
 * current_path - aktualny katalog roboczy
 */
void send_file(int control,char* file_name,int arg,char* mode,char current_path[])
{
  int bytesReceived;
  char recvBuff[256];
  char file[256];
  FILE* fp;
  
  //połączenie ścieżki z nazwą 
  (void)snprintf(file,255,"%s/%s",current_path,file_name);
  
  //próba otworzenia żądanego pliku
  fp = fopen(file, mode);
  if(NULL == fp)
  {
    //komunikat w przypadku niepowodzenia
    printf("Error opening file");
    (void)write(control,r_550,strlen(r_550));
  }
  else
  {
    //komunkat o powodzeniu
    (void)write(control,r_150,strlen(r_150));
    if(strcmp(mode,"rb+")==0)
    {
      //jeżeli plik jest otwierany w trybie binarnym
      while((bytesReceived = (int)fread(recvBuff,1,256,fp))>0)
      {
        (void)write(arg,recvBuff,(size_t)bytesReceived);
      }
    }
    else
    {
      //jeżli plik jest otwierany w trybie ascii
      while(fgets(recvBuff, 256,fp)!=NULL)
      {
        (void)write(arg,recvBuff,strlen(recvBuff));
      }
    }
    
    //zamknięcie pliki i komunikat o zakończeniu transferu
    (void)fclose(fp);
    (void)write(control,r_226,strlen(r_226));
  }
}

/*
 * Funkcja odbierająca plik od klienta
 * control - identyfikator połączenia kontrolnego
 * file_name - nazwa przesyłanego pliku
 * arg - identyfikator połączenia danych
 * current_path - aktualny katalog roboczy
 */
void recive_file(int control,char* file_name,int arg,char* mode,char current_path[])
{
  int bytesReceived;
  char recvBuff[256];
  char file[256];
  FILE *fp;
  
  //sprawdzenie, czy nazwa pliku zawiera pełną ścieżkę
  if(file_name[0]!='/')
    (void)snprintf(file, 255,"%s/%s",current_path,file_name);
  else 
    (void)snprintf(file, 255,"%s",file_name);
  
  
  //próba otworzenia pliku
  fp = fopen(file, mode);
  if(NULL == fp)
  {
    printf("Error opening file");
    (void)write(control,r_550,strlen(r_550));
  }
  else
  {
    (void)write(control,r_150,strlen(r_150));
    if(strcmp(mode,"wb+")==0)
    {
      //zapisywanie pliku w trybie binarnym
      while((bytesReceived = (int)read(arg, recvBuff, 256)) > 0)
      {
        (void)fwrite(recvBuff,1,(size_t)bytesReceived,fp);
      }
    }
    else
    {
      //zapisywanie pliku w trybie ascii
      while((bytesReceived = (int)read(arg, recvBuff, 256)) > 0)
      {
        (void)fputs(recvBuff,fp);
      }
    }
    //wysłanie komunikatu o zakończeniu transferu i zamknięcie pliku
    (void)write(control,r_226,strlen(r_226));
    (void)fflush(fp);
    (void)fclose(fp);
  }
}
