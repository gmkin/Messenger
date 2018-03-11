#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define KNRM  	"\x1B[0m"
#define KRED  	"\x1B[31m"
#define KGRN  	"\x1B[32m"
#define KYEL  	"\x1B[33m"
#define KBLU  	"\x1B[34m"
#define KMAG  	"\x1B[35m"
#define KCYN  	"\x1B[36m"
#define KWHT  	"\x1B[37m"

#define VERBOSE "\x1B[1;34m"
#define ERROR 	"\x1B[1;31m"
#define DEFAULT "\x1B[0m"

#define usrClr	KGRN
#define otrClr	KYEL

#define CRLFCRLF "\r\n\r\n"
#define CRLFCRLFNULL "\r\n\r\n\0"
#define MAX_CLIENTS 1024
#define BUFSIZE 1024
#define MAXNAME 10

#ifdef DBG 
	#define DEBUG(fmt, args...) fprintf(stderr, KMAG "DEBUG: " KGRN fmt KNRM, ##args)
#else
	#define DEBUG(fmt, args...) 
#endif

// Prints program help menu 
void usage();

// Prints error and quits
// msg - Message to print
void error(char *msg);

// 0 - Strings not equal; 1 - Strings are equal
int equals(char* str1, char* str2);

int max(int a, int b);

// ---------------------------------------------------------------------------------------------
// Read with verbose support
int vRead(int sockfd, char* buff, int max_buff, int vFlag, char* error_msg);

// To be used with server, returns protocol ending with \r\n\r\n, pass back same buff
char* bufRead(int fd, char* buff, int* buff_size, int vFlag, char* error_msg);

// Write with verbose support
void vWrite(int sockfd, char* buff, int vFlag);

// Establishes Connection with Server
void me2u(int sockfd, int vFlag);

// Prints client help options
void help();

// sends LOGIN PROTOCOL to sockfd
char* login(int sockfd, int vFlag, char* username, int* bufReadSize);

// lists all users on the server to talk to
void listU(int sockfd, int vFlag, char* buff, int* max_buff);

// logs the user out
void logout(int sockfd, int vFlag);

// sends a message protocol to server
void to(int sockfd, int vFlag, char* buf);

// processes message protocol to server
void from(int sockfd, int vFlag, char* buf);

// handles to and from, creates xterm if necessary
int message(int sockfd, char* username, int vFlag, char* buff);

// cleans up all open sockets
void cleanup(fd_set fd_list);

int createXTerm(char* name, char* username, int parent_fd, int child_fd);

void removeClient(int pid);

void invalidateClient(int pid);

void uoff(char* buff);
//-----------------CLIENT DATA STRUCTURE----------------------------------
struct client{ 
    char name[11];
    int socket;
    pid_t pid;
    uint8_t available;
};

extern struct client all_clients[];
extern int array_size;
extern int server_sock;
extern int glo_vFlag;
extern fd_set ready;
#endif
