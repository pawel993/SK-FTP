#include "helper_functions.h"
/*
 * HELPER_FUNCTIONS
 * FUNKCJE POMOCNICZE
 */

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

