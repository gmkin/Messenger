#include "chat.h"
#include "client.h"

int a2i(char* something) {
    int total = 0, i =0, sign = 1;
    if(*something == '-'){
        sign = -1;
        i++;
    } 
    for(; i < strlen(something);i++){
        char c = *something;
        if( (c < '0') || (c > '9'))
            return -1;
        total = total*10 + (c - 48);
    }
    return total*sign;
}

int main(int argc, char* argv[]){
	int s= a2i(argv[1]);
	char* username = argv[2];
	char* receiverName = argv[3];
    printf("Socket: %d\n Username: %s\n Receiver Name:%s\n",s,username,receiverName);
	fd_set ready, copy;
	int max_fd = s > fileno(stdin) ? s : fileno(stdin);
	FD_ZERO(&copy);
    FD_ZERO(&ready);
	FD_SET(s,&copy);
	FD_SET(fileno(stdin),&copy);
    	
	char buf[BUFSIZE];
	for( ; ; ) {
        memset(buf,0,BUFSIZE);
		ready = copy;
		if(select(max_fd+1, &ready, NULL, NULL, NULL) < 0){
			error("Chat - Error while processing select");
        }

        // USER
		if(FD_ISSET(s, &ready)){
			read(s, buf, BUFSIZE);
			char* name = strchr(buf, ' ')+1;
            char* message = strchr(name,' ');
            *(message++) = '\0';
            char* end = strchr(message, '\r');
            *end = '\0';
			printf(">%s: %s%s%s", name, usrClr, message, DEFAULT);
		}

		// OTHER PERSON
		if(FD_ISSET(fileno(stdin), &ready)){
			read(fileno(stdin), buf, BUFSIZE);
            //printf("< %s",buf);
            if(!strcmp(buf,"/close\n"))
                exit(0);
			int message_size = 3 + strlen(receiverName) + strlen(buf) + strlen(CRLFCRLF);
			char payload[message_size];
            memset(payload,0,message_size);
			strcat(payload, "TO ");
			strcat(payload, receiverName);
			strcat(payload, " ");
			strcat(payload, buf);
			strcat(payload, CRLFCRLF);
            write(s,payload,strlen(payload));
		}
	}
	return 0;
}
