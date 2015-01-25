#! /bin/bash
#kompilacja wersji jednoplikowej
#gcc SK_FTP.c -Wall -pthread -o SK_FTP
gcc -Wall main.c helper_functions.c transfer.c responds.c -pthread -o serverFTP