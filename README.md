# Messenger

## Client
	Build by 'make clean all' for normal outputs
		'make clean debug' for debug outputs in case you would like to change the protocols.
	This creates a client appication and a chat application that the client will need when it forks xterm.
	Run the client from the base directory by bin/client [-v] NAME IP PORT
		- -v 	verbose flag tells the client to print all incoming/outgoing protocols
		- NAME 	the username you want to be recongized as
		- IP 	ip address to connect to, supports IPv4 and IPv6
		- PORT	port that you would like to connect to
		
## Server
	Can use 'make run' to run server with default settings
	Run the server as 'python server/server.py [-v] PORT nWORKERS MOTD'
		- -v	verbose flag tells the server to print all incoming/outgoing protocols
		- PORT	port to connect to
		- nWORKERS	number of worker threads to spawn
		- MOTD	message to send to the client when first connected
	
	When the server is launched, it will print out the IP address that it is currently located at

## Docker
	From the base directory, run 'docker build -t d_server server' to create a docker image of server and tag it.
	Next, to run the container, run 'docker run --rm -t -i d_server'.
		--rm 	cleans up the contianer once it is finished running (by '/shutdown')
		-t 	displays text to terminal
		-i 	(MUST HAVE) makes the container interactive, so epoll does not go crazy
		d_server is the container to run
	As with earlier, when the server is launched, it will print out the IP address that it is currently located at.
	You can update the arguments in server/Dockerfile as with above.
