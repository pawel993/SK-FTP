#ifndef _HELPER_H
#define _HELPER_H
#include <sys/types.h>
//wyznaczenie numeru portu z komunikatu klienta
int num_to_port(int a, int b);

//inicjalizacja numerów portów
void initialize_ports(ushort passiv, ushort activ);

//zwraca numer portu dla kolejnego połączenia aktywnego
ushort active_port();

//zwraca numer portu dla kolejnego połącznia pasywnego
ushort passive_port();

//konwersja numeru portu do postaci przesyłanej klientowi
void port_to_num(char* server_addres,int a,char result[]);

#endif