# Messenger

## Client
	Build by 'make clean all' for normal outputs 'make clean debug' for debug outputs in case you would like to change the protocols.  This creates a client appication and a chat application that the client will need when it forks xterm.
	Run the client from the base directory by bin/client [-v] NAME IP PORT
		- -v 	verbose flag tells the client to print all incoming/outgoing protocols
		- NAME 	the username you want to be recongized as
		- IP 	ip address to connect to, supports IPv4 and IPv6
		- PORT	port that you would like to connect to
		
## Server
	Run the server as 'python server/server.py [-v] PORT nWORKERS MOTD'
		- -v	verbose flag tells the server to print all incoming/outgoing protocols
		- PORT	port to connect to
		- nWORKERS	number of worker threads to spawn
		- MOTD	message to send to the client when first connected
	
	When the server is launched, it will print out the IP address that it is currently located at
