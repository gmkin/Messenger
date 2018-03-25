#include "client.h"

// General purpose helper functions 
void usage(int exit_status) {
	printf("%s\n%s\n%s\n%s\n%s\n%s\n", 
		"./client [-hv] username SERVER_IP SERVER_PORT",
		"-h		Displays this help menu, and returns EXIT_SUCCESS.",
		"-v		Verbose print all incoming and outgoing protocol verbs and content.",
		"username		This is the userusername to display when chatting.",
		"SERVER_IP	The ip address of the server to connect to.",
		"SERVER_PORT	The port to connect to.");

	exit(exit_status);
}

void error(char *msg) {
	fprintf(stderr, "%s%s%s\n", KRED, msg, KNRM);
	exit(EXIT_FAILURE);
}

int equals(char* str1, char* str2) {
	if (*str1 != *str2)
		return 0;
	return (*str1 == '\0' && *str2 == '\0') || equals(str1 + 1, str2 + 1);
}

int max(int a, int b) {	
	return a > b ? a : b;
}

// ---------------------------------------------------------------
// prints protocol if vFlag is active
void printProtocol(char* msg, int vFlag) {
	if (vFlag)
		printf("Protocol: <%s%s%s>\n", VERBOSE, msg, DEFAULT);
}

// write wrapper, with verbose support
void vWrite(int sockfd, char* buff, int vFlag) {
	printProtocol(buff, vFlag);
	if (write(sockfd, buff, strlen(buff)) < 0) 
		error("ERROR writing to socket");
}

// raw read wrapper, with verbose support . returns characters read
int vRead(int sockfd, char* buff, int max_buff, int vFlag, char* error_msg) {
	int bytes_read = read(sockfd, buff, max_buff);
	if (bytes_read < 0 && errno != EINTR)
		error(error_msg);
	return bytes_read;
}

// returns one protocol ending with \r\n\r\n, remember to pass back same string
char* bufRead(int fd, char* buff, int* buff_size, int vFlag, char* error_msg) {
	if (buff == NULL) {
		*buff_size = BUFSIZE;
		buff = malloc(BUFSIZE);
        memset(buff,0,*buff_size);
	} else {
		int i = strlen(buff);
        char* temp = buff+i+1;
		memcpy(buff, temp, *buff_size-i);
		memset(buff + *buff_size - i, 0, i);
	}
	
	char* end;
    int count = 0;
	while ((end = strstr(buff, "\r\n\r\n")) == NULL) {
		if(count++){
            buff = realloc(buff, *buff_size + BUFSIZE);
            memset(buff + strlen(buff), 0, BUFSIZE);
            *buff_size += BUFSIZE;
        }
		int bytes_read = vRead(fd, buff, *buff_size, vFlag, error_msg);
		if (bytes_read == 0) {
			close(fd);
			error("Cannot connect to server, Client shutdown");
		} else if (bytes_read < 0 && errno != EINTR)
			error("vRead < 0 and not EINT; what error could this be?");
	}

	end += 4;									// include CRLFCRLF
    int len = strlen(buff);
	memmove(end + 1, end, len);
	*end = '\0';
	printProtocol(buff, vFlag);
	return buff;
}

void help() {
	printf(" %s\n\t%s\n\n %s\n\t%s\n\t%s\n\n %s\n\t%s\n\n %s\n\t%s\n\t%s\n %s\n\t%s\n", 
		"/help",
		"- Prints all the commands which the client accepts",
		"/logout",
		"- Initates the logout procedure with the server",
		"- Closes all chat windows and cleans up file descriptors",
		"/listu",
		"- Lists all users from the server",
		"/chat <to> <msg>",
		"- If the user exists, then spawn a window for further communication",
		"- If the user does not exist, prints error message",
		"/whoami",
		"- Displays username");
}

void me2u(int sockfd, int vFlag) {
	char buff[BUFSIZE];
	memset(buff, 0, BUFSIZE);
	memcpy(buff, "ME2U", 4);
	strcat(buff, CRLFCRLF);
	vWrite(sockfd, buff, vFlag);

	memset(buff, 0, BUFSIZE);
	vRead(sockfd, buff, BUFSIZE, vFlag, "ERROR reading from socket");

	if (!equals(buff, "U2EM\r\n\r\n"))
		error("Unrecongized result after sending inital ME2U");

	DEBUG("%s\n", "Initial handshake completed");
}

char* login(int sockfd, int vFlag, char* username, int* bufReadSize) {
	char buff[BUFSIZE];
	memset(buff, 0, BUFSIZE);
	memcpy(buff, "IAM ", 4);
	strcat(strcat(buff, username), CRLFCRLF);
	vWrite(sockfd, buff, vFlag);

	char* readBuff = bufRead(sockfd, NULL, bufReadSize, vFlag, "ERROR socket read");
	if (equals(readBuff, "ETAKEN\r\n\r\n"))
		error("Username Taken");
	if (!equals(readBuff, "MAI\r\n\r\n"))
		error("Expected username confirmation/rejection");

	readBuff = bufRead(sockfd, readBuff, bufReadSize, vFlag, "Unable to read MOTD");
	if (strncmp(readBuff, "MOTD ", 5))
		error("Expected MOTD");
	
	printf("%sMessage of the Day: %s%s\n", KGRN, readBuff + 5, KNRM);
	DEBUG("%s\n", "Successfully Logged In");
	return readBuff;
}

void listU(int sockfd, int vFlag, char* buff, int* max_buff) {
	vWrite(sockfd, "LISTU\r\n\r\n", vFlag);

	buff = bufRead(sockfd, buff, max_buff, vFlag, "Expecting UTSIL ..");
	if(!strncmp(buff,"UTSIL", 6))
		error("Expected UTSIL");
    char* name = strchr(buff,' ');
	printf("Users: %s%s%s\n", KYEL, name+1, KNRM);
}

void from(int sockfd, int vFlag, char* buf) {
	char sendBuff[6 + MAXNAME];			// 'FROM' + space + MAXNAME + NULL
	memset(sendBuff, 0, sizeof(sendBuff));
	memcpy(sendBuff, "MORF ", 5);

	int i = 5;
	for ( ; *(buf + i) != ' '; i++)
		*(sendBuff + i) = *(buf + i);
	memcpy(sendBuff + i, CRLFCRLF, strlen(CRLFCRLF));
	*(sendBuff + i + strlen(CRLFCRLF)) = '\0';
	vWrite(sockfd, sendBuff, vFlag);
}

int createXTerm(char* name, char* username,  int parent_fd, int child_fd){
	pid_t pid = fork();
	if(pid == 0){
		close(parent_fd);
		char* args = "/usr/bin/xterm";
		char child[4] = {0};
		sprintf(child,"%d",child_fd);
		execl(args, args, "-e", "./bin/chat", child, username, name, (char*)0);
		error(strerror(errno));
	}
	return pid;
}

int message(int sockfd, char* username,int vFlag, char* buff) {
	DEBUG("IN MESSAGE with %s\n", buff);
	int found = 0, size_of_name = 1, count = 0, OT = 0;
	pid_t pid = 0;

	char* startOfName = strchr(buff, ' ') + 1;
	char* temp = startOfName;
	while(*(++temp) != ' '){
        if(*(temp-1) == '\0'){
            size_of_name=temp-startOfName-5;
            break;
        }
		size_of_name++;
    }
	char name[MAXNAME + 1];
	memset(name, 0, MAXNAME + 1);
	memcpy(name, startOfName, size_of_name);
	name[size_of_name + 1] = '\0';

	struct client available_client;
	memset(&available_client, 0, sizeof(struct client));
	while(count < array_size) {
		available_client = all_clients[count];
		DEBUG("Looking for client. Array size: %d Name: %s Looking for:%s With avail of: %d\n", array_size, available_client.name, name, available_client.available);
		if((available_client.available != 0) && !strcmp(available_client.name, name)) {
			DEBUG("FOUND Client");
			found = 1;
			break;
		}
		count++;
	}
	if(!found){
		DEBUG("Not found\n");
		memcpy(available_client.name, name, size_of_name);
		available_client.available = 1;
	}
 
	if(!strncmp(buff, "FROM ", 5)){
		if(!found){
			int my_sockets[2];
			socketpair(AF_LOCAL, SOCK_STREAM, 0, my_sockets);
			pid = createXTerm(name, username, my_sockets[0], my_sockets[1]);
			close(my_sockets[1]);
			available_client.socket = my_sockets[0];
			available_client.pid = pid;
		}
		vWrite(available_client.socket, buff, vFlag);
		from(sockfd, vFlag, buff);
        OT = 1;
	} else if(!strncmp(buff, "TO ", 3)){
		vWrite(sockfd, buff, vFlag);
		memset(buff, 0, BUFSIZE);
        available_client.available = found ? 4 : 3;
        all_clients[count] = available_client;
	} else if(!strncmp(buff, "OT ", 3)){
        //Did not ask for OT
        if(available_client.available != 3 && available_client.available != 4){
            raise(SIGTERM);
        }
		if(available_client.available == 3){
            int my_sockets[2];
			socketpair(AF_LOCAL, SOCK_STREAM, 0, my_sockets);
			pid = createXTerm(name,username, my_sockets[0], my_sockets[1]);
			available_client.socket = my_sockets[0];
			available_client.pid = pid;
			close(my_sockets[1]);
        }
        available_client.available = 1;
        all_clients[count] = available_client;
        OT = 1;
    }
    else if(!strncmp(buff, "EDNE ", 5)){
        //Did not ask for EDNE
        if(available_client.available != 3 && available_client.available != 4){
            raise(SIGTERM);
        }
        available_client.pid= -1;
        available_client.socket = 0;
        available_client.available = 0;
        all_clients[count] = available_client;
        removeClient(-1);
    }
	// CHAT	- Chat to same person should be sent to them
	else if (!strncmp(buff,"/chat ",6)){
		DEBUG("CHAT\n");

		char* data = strchr(buff, ' ');
		char newMessage[2 + strlen(data) + strlen(CRLFCRLF) + 1];
		memset(newMessage, 0, 2 + strlen(data) + strlen(CRLFCRLF) + 1);
		strcat(strcat(strcat(newMessage,"TO"), data), CRLFCRLF);
		vWrite(sockfd, newMessage, vFlag);
        available_client.available = found ? 4: 3;
        all_clients[count] = available_client;
        OT = 1;
	}

	if(!found)
		all_clients[array_size++] = available_client;
    if((available_client.available==3 || available_client.available==4) && found){
        DEBUG("CLEARING");
        FD_CLR(available_client.socket,&ready);
    }
	return OT ? available_client.socket : 0;
}

void logout(int sockfd, int vFlag) {
	vWrite(sockfd, "BYE\r\n\r\n", vFlag);

	char* buff;
    int size = BUFSIZE;
	do{
        buff = bufRead(sockfd, NULL, &size, vFlag, "Expecting EYB from server");
    } while(!equals(buff, "EYB\r\n\r\n"));

}

void cleanup(fd_set fd_list) {
	// TBD, close all sockets (except to main)
}

void removeClient(int pid){
	int count = 0;
	while(count < array_size){
		struct client cl = all_clients[count++];
		if(cl.pid == pid){
			struct client temp = all_clients[--array_size];
			DEBUG("Removing client: %s (fd: %d) Array size:%d\n", cl.name, cl.socket, array_size);
			all_clients[count - 1] = temp;
			break;
		}
	}
}

void invalidateClient(int pid){
	int count = 0;
	while(count < array_size){
		struct client* cl = &(all_clients[count++]);
		if(cl->pid == pid){
			cl->available = 2; 
			DEBUG("Invalidating client: %s\n", cl->name);
			break;
		}
	}
}

void uoff(char* buff){
    int count = 0;
    char* startOfName = strchr(buff,' ')+1;
    *(startOfName+strlen(startOfName)-4) = '\0';
    while(count < array_size){
        struct client* temp = &(all_clients[count++]);
        if(!strcmp(temp->name,startOfName)){
            invalidateClient(temp->pid);
            break;
        }
    }    
    *(startOfName+strlen(startOfName)) = '\r';
}
