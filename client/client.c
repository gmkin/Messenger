#include "client.h"
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

int array_size = 0;
struct client all_clients[MAX_CLIENTS] = {0};
int glo_vFlag = 0;
int server_sock = 0;
fd_set ready;

void child_handler(int signum){
	int status;
	int pid;

	while((pid = waitpid(WAIT_ANY, &status, WNOHANG)) > 0){
//		DEBUG("Reaped child");
		invalidateClient(pid);
	}

//	if(pid == -1)
//		perror("ERROR");
}

void terminate_handler(int signum){
	logout(server_sock, glo_vFlag);
	close(server_sock);
	for(int i = 0; i < array_size; i++) {
		struct client temp = all_clients[i];
		DEBUG("Closing socket %d\n",temp.socket);
		close(temp.socket);
	}
	exit(0);
}

// validates arguments and exits if such exists
void validateArgs(int argc, char** argv, int* vFlag) {
	if (argc < 2)	// at the very least -h
		usage(EXIT_FAILURE);
	
	if (equals(*(argv + 1), "-h"))
		usage(EXIT_SUCCESS);

	*vFlag = equals(*(argv + 1), "-v");

	if (argc != (4 + *vFlag))
		usage(EXIT_FAILURE);

	if (strlen(*(argv + *vFlag + 1)) > 10)
		error("Username is limited to 10 characters");
}

void prepareConnection(int argc, char** argv, int* sock){
		struct addrinfo hints;
		struct addrinfo *result, *rp;

		if (argc < 3) {
			fprintf(stderr, "Usage: %s host port msg...\n", argv[0]);
			exit(EXIT_FAILURE);
		}

		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_UNSPEC;    
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_protocol = 0;

		int s = getaddrinfo(argv[argc - 2], argv[argc - 1], &hints, &result);
		if (s != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
			exit(EXIT_FAILURE);
		}

		for (rp = result; rp != NULL; rp = rp -> ai_next) {
			*sock = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
			if (*sock == -1)
				continue;

			if (connect(*sock, rp -> ai_addr, rp -> ai_addrlen) != -1)
				break;

			close(*sock);
		}

		if (rp == NULL)
			error("Could not connect");
}

void setUpClient(int argc, char** argv, int* vFlag) {
	struct sigaction oldact, newact; 
	newact.sa_handler = child_handler; 
    newact.sa_flags = SA_RESTART;
	sigemptyset(&newact.sa_mask);
	sigaddset(&newact.sa_mask, SIGTERM);
	sigaddset(&newact.sa_mask, SIGINT);
	sigaction(SIGCHLD,&newact, &oldact);

	struct sigaction closeact, oldclose;
	closeact.sa_handler = terminate_handler;
	sigemptyset(&closeact.sa_mask);
	sigaddset(&closeact.sa_mask, SIGCHLD);
	sigaction(SIGTERM, &closeact,&oldclose);
	sigaction(SIGINT, &closeact,&oldclose);

	validateArgs(argc, argv, vFlag);
	glo_vFlag = *vFlag;
}

// 0 - cannot find 2 space, 1 - found it
int checkForTwoSpace(char* buf) {
	char* spacePtr = strchr(buf, ' ');
	if (spacePtr == NULL)
		return 0;

	// assumes no double spaces
	return strchr(spacePtr + 1, ' ') != NULL;
}

int main(int argc, char **argv) {
	// verbose flag: 0- Do not print protocols; 1- Print protocols
	int sockfd, buffReadSize = 0, vFlag = 0;
	setUpClient(argc, argv, &vFlag);

	char* username = *(argv + vFlag + 1);
	prepareConnection(argc, argv, &sockfd); 
	server_sock = sockfd;

	me2u(sockfd, vFlag);
	char* buffRead = login(sockfd, vFlag, username, &buffReadSize);

	// I hope we dont confuse these..
	fd_set read;			// ready - set fd; read - one for select to touch
	int maxfd = max(sockfd, fileno(stdin)) + 1;	// which is probably always sockfd
	FD_ZERO (&ready);
	FD_SET (fileno(stdin), &ready);		// user inputs
	FD_SET (sockfd, &ready);			// read from server

	char buf[BUFSIZE];
	for ( ; ; ) {
		memset(buf, 0, BUFSIZE);
		read = ready;
		DEBUG("Waiting on I/O\n");
		if (select (maxfd, &read, NULL, NULL, NULL) < 0){
			//perror ("Error while processing select");
            if(errno ==EINTR)
                continue;
        }

		if (FD_ISSET (fileno(stdin), &read)) {
			vRead(fileno(stdin), buf, BUFSIZE, 0, "Fail to read from stdin");

			if (!strncmp(buf, "/listu", 6))
				listU(sockfd, vFlag, buffRead, &buffReadSize);
			else if (!strncmp(buf, "/chat ", 6)) {
				if (!checkForTwoSpace(buf)) {
					fprintf(stderr, "%sExpected /chat <name> <msg>%s\n", KRED, KNRM);
					continue;
				}

				int new_fd = message(sockfd, username, vFlag, buf);
				if(new_fd){
					FD_SET(new_fd, &ready);
					maxfd = max(new_fd, maxfd) + 1;
				}
			} else if (!strncmp(buf, "/logout", 7)) {
				logout(sockfd, vFlag);
				break;
			} else if (!strncmp(buf, "/whoami", 7))
				printf("Username: %s\n", username);
			else if (!strncmp(buf, "/help", 5))
				help();
			else
				printf("%s%s%s\n", KCYN, "Cannot understand input, type '/help' for commands", KNRM);
		}

		if (FD_ISSET (sockfd, &read)) {
			buffRead = bufRead(sockfd, buffRead, &buffReadSize, vFlag, "Awaiting data");
			DEBUG("SERVER DATA <%s>\n", buffRead);

			if ((!strncmp(buffRead, "FROM ", 5)) || !strncmp(buffRead,"OT ",3)) {
				int new_fd = message(sockfd, username, vFlag, buffRead);
				if(new_fd) {
					FD_SET(new_fd, &ready);
					maxfd = max(new_fd, maxfd)+1;
				}
			}
            else if (!strncmp(buffRead, "EDNE ", 5)){
                message(sockfd,username, vFlag, buffRead);
            }
			else if (!strncmp(buffRead, "UOFF ", 5)) {
				DEBUG("%s%s has logged off%s\n", KCYN, buffRead + 5, KNRM);

				// TODO: RYAN - INSERT CLOSE XTERM for user
                uoff(buffRead);
			}
			else {
				printf("%s%s%s\n", KCYN, "Cannot understand server data", KNRM);
				break;
			}
		}

		//XTerm parsing
		int count = 0;
		while(count < array_size){
			struct client cl = all_clients[count++];
			DEBUG("COUNT %d CHECKING %s WITH SOCKET NUMBER %d\n", count, cl.name, cl.socket);
            if(cl.socket == 0)
                continue;
			if(cl.available == 2) {
				DEBUG("REMOVING %s\n", cl.name);
				removeClient(cl.pid);
				FD_CLR(cl.socket, &ready);
                if(cl.socket)
                    close(cl.socket);
				continue;
			}
			if(FD_ISSET(cl.socket, &read)){
				DEBUG("OTHER FD? XTERM: %s\n", username);
				memset(buf, 0, BUFSIZE);
				if(vRead(cl.socket, buf, BUFSIZE, 0, "Chat gave poorly formatted TO") > 0)
					message(sockfd, username, vFlag, buf);
			}
		}
	}

	cleanup(ready);
	close(sockfd);
	printf("%sThank you for using our client (: \n%s", KCYN, KNRM);
	return EXIT_SUCCESS;
}
