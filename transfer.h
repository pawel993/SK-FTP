#ifndef _TRANSFER_H
#define _TRANSFER_H

/*
 * TRANSEFR
 * FUNKCJE DO TRANSFERU PLIKÓW
 */

//wysłanie pliku do klienta
void send_file(int control,char* file_name,int arg,char* mode,char current_path[]);

//pobranie pliku od klienta
void recive_file(int control,char* file_name,int arg,char* mode,char current_path[]);

#endif