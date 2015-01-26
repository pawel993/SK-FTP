#include "responds.h"

/*
 * RESPONDS
 * ODPOWIEDZI SERWERA
 * 
 */
char* r_220="220 Service ready for new user.\n";
char* r_215="215 Unix system type.\n";
char* r_250="250 Requested file action okay, completed.\r\n";
char* r_331="331 User name okay, need password.\n";
char* r_230="230 User logged in, proceed.\n";
char* r_200="200 Command okay.\n";
char* r_500="500 Syntax error, command unrecognized.\n";
char* r_502="502 Command not implemented.\n";
char* r_150="150 File status okay; about to open data connection.\n";
char* r_226="226 Closing data connection.\n";
char* r_550="550 Requested action not taken.File unavailable.\n";
char* r_425="425 Can't open data connection.\n";