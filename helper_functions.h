#ifndef _HELPER_H
#define _HELPER_H

//wyznaczenie numeru portu
int num_to_port(int a, int b);


//konwersja numeru portu do postaci przesyłanej klientowi
void port_to_num(char* server_addres,int a,char result[]);

#endif