#include "helper_functions.h"
#include <stdio.h>
#include <string.h>
/*
 * HELPER_FUNCTIONS
 * FUNKCJE POMOCNICZE
 */

//ostatni wykorzystany numer portu połączenia pasywnego
static ushort passive_p;

//ostatni wykorzystany numer portu połączenia aktywnego
static ushort active_p;

/*
 * a^b
 * 
 */
static int power(int a, int b)
{
  int temp=1;
  int i;
  for(i=0;i<b;i++){
    temp*=a;
    }
  return temp;
}

/*
 * zwraca kolejny numer portu dla połączenia aktywnego
 * 
 */
ushort active_port()
{
  active_p++;
  return active_p;
}

/*
 * zwraca kolejny numer portu dla połączenia pasywnego
 * 
 */
ushort passive_port()
{
    passive_p++;
    return passive_p;
}

/*
 * funkcja inicjalizująca początkowe numery portów
 * dla połaczenia aktywnego i pasywnego
 */
void initialize_ports(ushort passiv, ushort activ)
{
  passive_p=passiv;
  active_p=activ;
}

/*
 * konwersja wartości z komunikatu klienta na numer portu
 * 
 */
int num_to_port(int a, int b)
{
  unsigned int u_a = (unsigned int) a;
  unsigned int u_b = (unsigned int) b;
  int c;
  unsigned int k;
  for(c=8;c>=0;c--)
  {
    k = u_a>>(unsigned int)c;
    if((k&1)==1)
    {
      u_b+=power(2,c+8);
    }
  }
  return (int)u_b;
}

/*
 * konwersja numeru portu na format przesyłany klientowi
 * 
 */
void port_to_num(char* server_addres,int a,char result[])
{
  unsigned int u_a = (unsigned int) a;
  char temp[60];
  int c;
  unsigned int k;
  int r1=0,r2=0;
  for(c=16;c>=0;c--)
  {
    k=u_a>>(unsigned int)c;
    if(((k&1)==1) && (c>8))
    {
      r1+=power(2,c-8);
    }
    else if((k&1)==1) 
    {
      r2+=power(2,c);
    }
  } 
  strncpy(temp,server_addres,strlen(server_addres));
  for(c=0;c<(int)strlen(server_addres);c++)
  {
    if(temp[c]=='.')
      temp[c]=',';
  }
  (void)snprintf(result, 256,"(%s,%d,%d).\n",temp,r1,r2);
}

