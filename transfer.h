#ifndef _TRANSFER_H
#define _TRANSFER_H


void send_file(void* control,char* file_name,int arg,char* mode,char current_path[]);

void recive_file(void* control,char* file_name,int arg,char* mode,char current_path[]);

#endif