#include "transfer.h"
#include "responds.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void send_file(void* control,char* file_name,int arg,char* mode,char current_path[])
{
  int bytesReceived;
  char recvBuff[256];
  char file[256];
  sprintf(file,"%s/%s",current_path,file_name);
  FILE* fp;
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

void recive_file(void* control,char* file_name,int arg,char* mode,char current_path[])
{
  int bytesReceived;
  char recvBuff[256];
  char file[256];
  if(file_name[0]!='/')
    sprintf(file,"%s/%s",current_path,file_name);
  else 
    sprintf(file,"%s",file_name);
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
