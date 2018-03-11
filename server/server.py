from socket import *
from fcntl import fcntl, F_GETFL, F_SETFL
from queue import Queue
import sys, time, select, os, threading, traceback

global vFlag			# True to enable verbosely print protocols
global messageOfTheDay	# Message of the Day, prints when client first connects

debug = False			# True to enable debug mode
keepAlive = True		# keeps the server/threads alive
clientLock = threading.Lock()
clientList = {}			# list of clents: fd -> (socket, name)
nameToFd = {} 
threadList = []			# list of all the threads to close them afterwards
eventQueue = Queue()	# (socket, isNewClient) sock to process .. (None, None) = kill


class worker(threading.Thread):
	""" worker thread to handle events in eventQueue """
	def __init__(self, threadID):
		""" initialize worker thread """
		threading.Thread.__init__(self)
		self.threadID = threadID
		dPrint("Thread {} started".format(threadID))
	
	def run(self):
		""" running instance of worker """
		global eventQueue
		global clientList

		while keepAlive:
			socket, isNewClient = eventQueue.get()
			if socket is None or isNewClient is None:
				dPrint("Thread {} Terminated".format(self.threadID))
				break

			if isNewClient:
				processNewClient(socket)
				continue
			
			line = vRead(socket)
			if len(line) == 0:
				removeClient(socket.fileno())
				continue

			foundEnd = False		# no more to read from this buffer
			while keepAlive and not foundEnd:
				if socket.fileno() < 0:
					break

				while keepAlive and not foundEnd and "\r\n\r\n" not in line:
					newMsg = vRead(socket)
					if len(newMsg) == 0:
						foundEnd = True
						break
					line += newMsg

				if len(line) == 0:
					break
				if "\r\n\r\n" not in line:
					print('OBTAINED LINE WITHOUT CRLFCRLF > {}'.format(line))
					break					# basically ignores this read
				protocol, line = line.split("\r\n\r\n", 1)

				if debug:
					print("\nOn thread {} (fd:{}) job:{}".format(
						self.threadID, socket.fileno(), protocol))
				processProtocol(socket, protocol)
				if len(line) == 0:
					break			# risky break if protocol same len as buffer
		

def dPrint(msg):
	""" prints if debug flag is active """
	global debug

	if debug:
		print("DEBUG: {}".format(msg))


def vPrint(fd, msg):
	""" prints if vFlag is active """
	global vFlag
	global debug

	if not msg[-4:] == '\r\n\r\n':
		print('Protocol <{}> DOES NOT end in CRLFCRLF'.format(msg))
		exit(1)

	if vFlag:
		if debug and str(fd) in clientList:
			print("Verbose Protocol (fd:{} name:{}): <{}>"
				.format(fd, clientList[str(fd)][1], msg[:-4]))
		else:
			print("Verbose Protocol: <{}>".format(msg[:-4]))


def uTest(expected, got, toClose=None):
	""" unit tests io, toClose-socket to disconnect if invalid """
	global debug

	if debug and not expected == got:
		print('Expected: <{}>, Got: <{}>'.format(expected, got))
		if toClose is not None:
			toClose.close()


def usage(path, exitStatus):
	""" prints usage for this program """
	print('usage: python {} [-h|v] PORT_NUMBER NUM_WORKERS MOTD'.format(path))
	print(' {}\t\t{}\n {}\t\t{}\n {}\t{}\n {}\t{}\n {}\t\t{}\n'.format(
		'-h', 'Displays this help menu',
		'-v', 'Verbose print all incoming and outgoing protocol verbs and content',
		'PORT_NUMBER', 'Port number to listen on.',
		'NUM_WORKERS', 'Number of workers to spawn',
		'MOTD', 'Message to display to the client when they connect'))
	sys.exit(exitStatus)


def validateArgs(args):
	""" validates args, quits if necessary """
	global vFlag

	if len(sys.argv) < 2:
		usage(args[0], 1)

	if args[1] == "-h":
		usage(args[0], 0)

	vFlag = args[1] == '-v'
	if not len(args) == (5 if vFlag else 4):
		print('Invalid number of arguments')
		usage(args[0], 1)

	if (not args[-2].isdigit()):			# checks if postive num
		print('Invalid number of threads')
		usage(args[0], 1)


def prepareConnection(args):
	""" sets up server and epoll """
	global vFlag

	host = 'localhost'
	with socket(AF_INET, SOCK_DGRAM) as temp:
		temp.connect(('8.8.8.8', 1))
		host = temp.getsockname()[0]

	server = socket(AF_INET, SOCK_STREAM)
	server.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)	# reuse sockets
	server.bind((host, int(sys.argv[2 if vFlag else 1])))
	server.listen(5)

	epoll = select.epoll()
	epoll.register(server.fileno(), select.EPOLLIN)
	
	fd_stdin = sys.stdin.fileno()
	fcntl(fd_stdin, F_SETFL, fcntl(fd_stdin, F_GETFL) | os.O_NONBLOCK)
	epoll.register(fd_stdin, select.EPOLLIN)

	print('Server at {}'.format(host))
	return server, epoll


def setupServer(argv):
	""" prepres server environment """
	global messageOfTheDay
	global threadList
	
	validateArgs(argv)
	messageOfTheDay = argv[-1]
	for i in range(int(argv[-2])):
		currThread = worker(i)
		currThread.start()
		threadList.append(currThread)

	return prepareConnection(argv)


def vRead(socket):
	""" read from socket """
	data = ""
	try:
		data = socket.recv(1024)
	except Exception as e:
		dPrint(traceback.extract_stack())
		dPrint('On fd {}, {}'.format(socket.fileno(), e))
		if e.errno == 104:		# connection reset by peer
			removeClient(socket.fileno())
	return "" if len(data) == 0 else data.decode()


def rWrite(socket, msg):
	""" raw write msg to socket outside of epoll """
	socket.send(msg.encode())
	vPrint(socket.fileno(), msg)


def vWrite(socket, msg):
	""" write msg to socket within epoll """
	epoll.modify(socket, select.EPOLLOUT)
	rWrite(socket, msg)
	epoll.modify(socket, select.EPOLLIN | select.EPOLLET)


def processNewClient(newSocket):
	""" adds new client to clientList """
	global debug

	dPrint('New client on fd: {}'.format(newSocket.fileno()))
	data = vRead(newSocket)
	vPrint(newSocket.fileno(), data)

	if not data == 'ME2U\r\n\r\n':
		uTest('ME2U\r\n\r\n', data, toClose=newSocket)
		return

	rWrite(newSocket, "U2EM\r\n\r\n")
	data = vRead(newSocket)
	vPrint(newSocket.fileno(), data)
	if not data[:4] == "IAM ":
		uTest('IAM <name>\r\n\r\n', data, toClose=newSocket)
		return

	name = data[4:-4]		# chops off IAM and CRLFCRLF
	for fd in clientList:
		if name == clientList[fd][1]:
			dPrint("------------------fd: {}".format(clientList[fd][0].fileno()))
			rWrite(newSocket, "ETAKEN\r\n\r\n")
			dPrint("Name already taken")
			newSocket.close()
			return

	rWrite(newSocket, "MAI\r\n\r\n")

	# FINDME: update tuple to [newSocket, name, socketMutex]	# [inc_buff, OT_Flag
	with clientLock:
		clientList[str(newSocket.fileno())] = [newSocket, name, threading.Lock()]
		nameToFd[name] = newSocket;

	rWrite(newSocket, "MOTD {}\r\n\r\n".format(messageOfTheDay))
	epoll.register(newSocket.fileno(), select.EPOLLIN | select.EPOLLET)
	newSocket.setblocking(0)	# if earlier, blocking io error


def removeClient(fd):
	""" Removes and cleans up client from epoll and clientList """
	socket, name = clientList[str(fd)][:2]
	dPrint("Disconnecting user <{}> on fd: {}".format(name, fd))
	del nameToFd[name]
	del clientList[str(fd)]
	epoll.unregister(socket.fileno())
	socket.close()


def handleSystemInput():
	global keepAlive

	data = sys.stdin.read(10)[:-1]			# chop off newline
	if data == '/users':
		if len(clientList) == 0:
			print("It is a lonely world: no users")
		for fd in clientList:
			user = clientList[fd]
			print("user:{} on fd:{}".format(user[1], user[0].fileno()))
	elif data == '/help':
		print('HELP MENU:\n {}\t\t{}\n {}\t\t{}\n {}\t{}\n'.format(
			'/users', '- dumps a list of current logged in users',
			'/help', '- prints all commands the server accepts',
			'/shutdown', '- shuts down the server by disconnecting all users'))
	elif data == '/shutdown':
		keepAlive = False
		for _ in range(len(threadList)):	# tell threads to end
			eventQueue.put((None, None))

		for t in threadList:				# reap threads
			t.join()

		for fd in clientList:				# disconnecting everyone
			# vWrite(clientList[str(fd)][0], "EYB\r\n\r\n")
			dPrint('Disconnecting user <{}> on fd:{}'
				.format(clientList[fd][1], clientList[fd][0].fileno()))
			epoll.unregister(int(fd))
			clientList[fd][0].close()
	else:
		print('Cannot understand server input, type "/help" for options')


def processProtocol(socket, protocol):
	""" Processes one protocol, buffering handled in worker thread """
	global clientList

	datam = protocol
	fd = socket.fileno()
	sender = clientList[str(fd)][1]

	vPrint(socket.fileno(), "{}\r\n\r\n".format(datam))        # print protocol
	if datam == 'LISTU':
		vWrite(socket, "UTSIL {}\r\n\r\n".format(
			' '.join([values[1] for values in clientList.values()])))
	elif datam[:3] == 'TO ':
		toName, toMsg = datam.split(" ", 2)[1:]
		for fd_c in clientList:
			if clientList[fd_c][1] == toName:
				vWrite(clientList[fd_c][0], 
					"FROM {} {}\r\n\r\n".format(sender, toMsg))
				break
		else:
			vWrite(socket, "EDNE {}\r\n\r\n".format(toName))
	elif datam[:5] == "MORF ":
		vWrite(nameToFd[datam.split(" ", 2)[1]], "OT {}\r\n\r\n".format(sender))
	elif datam == 'BYE':
		with clientLock:
			vWrite(socket, "EYB\r\n\r\n")
			removeClient(fd)

			for fd in clientList:
				vWrite(clientList[str(fd)][0], "UOFF {}\r\n\r\n".format(sender))
	else:
		dPrint("Cannot understand <{}>".format(datam))
		removeClient(fd)


if __name__ == "__main__":
	server, epoll = setupServer(sys.argv)
	try:
		while keepAlive:
			events = epoll.poll(1)
			for filefd, event in events:
				if filefd == sys.stdin.fileno():
					handleSystemInput()
				elif filefd == server.fileno():					# new client
					newSocket, addr = server.accept()
					eventQueue.put((newSocket, True))
				elif event & select.EPOLLIN:					# incoming data to read
					eventQueue.put((clientList[str(filefd)][0], False))
	finally:
		del clientList
		del nameToFd
		epoll.unregister(server.fileno())
		epoll.unregister(sys.stdin.fileno())
		epoll.close()
		server.close()
