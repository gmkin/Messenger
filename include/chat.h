#ifndef CHAT_H
#define CHAT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

#define BUFSIZE 1024
#define CRLFCRLF "\r\n\r\n"

#ifdef DBG 
	#define DEBUG(fmt, args...) fprintf(stderr, KMAG "DEBUG: " KGRN fmt KNRM, ##args)
#else
	#define DEBUG(fmt, args...) 
#endif

// mysterious funtion from ryan
int a2i(char* input);


#endif