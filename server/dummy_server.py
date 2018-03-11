from socket import socket, gethostname, AF_INET, SOCK_STREAM
import sys, time

global vFlag

def usage(exit_status):
	print('{}\n{}\n{}\n{}\n{}\n{}\n'.format(
		'python server/server.py [-h|v] PORT_NUMBER NUM_WORKERS MOTD',
		'-h		Displays help menu',
		'-v		Verbose print all incoming and outgoing protocol verbs and content',
		'PORT_NUMBER	Port number to listen on.',
		'NUM_WORKERS	Number of workers to spawn',
		'MOTD		Message to display to the client when they connect'))
	sys.exit(exit_status)


if __name__ == "__main__":
	if len(sys.argv) < 2:
		usage(1)

	# if sys.argv[1] == "-h":
	# 	usage(0)

	# vFlag = sys.argv[1] == '-v'
	# if len(sys.argv) < (vFlag ? 5 : 4):
	# 	usage(1)

	server = socket(AF_INET, SOCK_STREAM)
	# host = gethostname()
	# print(host)
	server.bind(('localhost', int(sys.argv[1])))
	server.listen(5)

	client, addr = server.accept()
	print('Connection from {}'.format(str(addr)))

	try:
                print('<{}>'.format(client.recv(1024).decode()))
                client.send('U2EM\r\n\r\n'.encode())

                print('<{}>'.format(client.recv(1024).decode()))
                client.send('MAI\r\n\r\n'.encode())

                client.send('MOTD Welcome to the server\r\n\r\n'.encode())

                time.sleep(3)
                client.send('FROM Ryan Hiryan\r\n\r\n'.encode())
                time.sleep(10)
		#print('<{}>'.format(client.recv(1024).decode()))
		#client.send('UTSIL RYAN1 RRYAN2 RYAN3 RYAN4 RYAN5 RYAN6 RYAN7\r\n\r\n'.encode())

		#print('<{}>'.format(client.recv(1024).decode()))
		#client.send('FROM RYAN1 RRYAN2 RYAN3 RYAN4 RYAN5 RYAN6 RYAN7\r\n\r\n'.encode())
	finally:
		client.close()
		print("CLOSING SOCKET")
		server.close()
