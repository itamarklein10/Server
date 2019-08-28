#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS


#include "httpParser.h"


struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[512];
	int len;
};

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;
struct timeval timeout;

void main()
{
	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(TIME_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Time Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	timeout.tv_sec = 120;
	timeout.tv_usec = 0;
	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeout);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Time Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			if (setsockopt(id, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
				cout << "Time Server: Error at setsockopt: " << WSAGetLastError() << endl;
				return false;
			}

			return true;
		}
	}
	return false;
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr *)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Time Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		if (WSAGetLastError() == WSAETIMEDOUT) {
			cout << "Time Server: Error at recv(): Too long to receive, time out triggered." << endl;
		}
		else {
			cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		}
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of \"" << &sockets[index].buffer[len] << "\" message.\n";
		sockets[index].len += bytesRecv;
		string req, body;
		map<string, string> reqContent;
		if (sockets[index].len > 0)
		{
			parseHttpRequest(sockets[index].buffer, &sockets[index].len, req, reqContent, body);
			sockets[index].len = 0;
			strcpy(sockets[index].buffer, "\0");
			sockets[index].send = SEND;
			try
			{
				sockets[index].sendSubType = parseHttpMethod(req);
			}
			catch (const char* exp)
			{
				sockets[index].sendSubType = ERROR_REQUEST;
			}
			string ans;

			if (sockets[index].sendSubType == GET)
				ans = GetAnswer(req);
			else if (sockets[index].sendSubType == PUT)
				ans = PutAnswer(req, body);
			else if (sockets[index].sendSubType == OPTIONS)
				ans = OptionsAnswer();
			else if (sockets[index].sendSubType == HEAD)
				ans = HeadAnswer(req);
			else if (sockets[index].sendSubType == _DELETE)
				ans = DeleteAnswer(req);
			else if (sockets[index].sendSubType == TRACE)
				ans = TraceAnswer(req);
			else if (sockets[index].sendSubType == EXIT)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}
			else
			{
				ans = req + " method not supported.";
			}

			for (int i = 0; i < ans.size(); ++i) sockets[index].buffer[i] = ans[i];
			sockets[index].buffer[ans.size()] = 0;
			return;
		}
	}
}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[256];

	SOCKET msgSocket = sockets[index].id;
	strcpy(sendBuff, sockets[index].buffer);
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Time Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

	sockets[index].send = IDLE;
}

void parseHttpRequest(char* recvBuff, int* buffSize, string& req, map<string, string>& content, string& body)
{
	string tempBuff(recvBuff);
	req = getLine(tempBuff);
	string currLine = getLine(tempBuff);
	while (currLine.size() > 0)
	{
		content.insert(parseHttpLine(currLine));
		currLine = getLine(tempBuff);
	}
	body.append(tempBuff);
}

string getLine(string& buffer)
{
	string resultString(buffer);
	if (buffer.find('\n') == string::npos)
	{
		buffer = "\0";
		return resultString;
	}

	resultString = buffer.substr(0, buffer.find('\n'));
	buffer = buffer.substr(buffer.find('\n') + 1, buffer.size());
	return resultString;
}

pair<string, string> parseHttpLine(string line)
{
	pair<string, string> reqCont;
	if (line.find(':') != string::npos)
	{
		reqCont.first = line.substr(0, line.find(':'));
		if (line.find(' ') != string::npos)
			reqCont.second = line.substr(line.find(' '), line.size());
		else
			reqCont.second = "";
	}
	else
	{
		reqCont.first = line; reqCont.second = "";
	}

	return reqCont;
}

int parseHttpMethod(string request)
{
	int result = -1;
	if (request.compare("exit") == 0)
		return EXIT;

	string requestType = request.substr(0, request.find(' '));
	if (requestType.compare("GET") == 0)
		result = GET;

	if (requestType.compare("PUT") == 0)
		result = PUT;

	if (requestType.compare("DELETE") == 0)
		result = _DELETE;

	if (requestType.compare("OPTIONS") == 0)
		result = OPTIONS;

	if (requestType.compare("HEAD") == 0)
		result = HEAD;

	if (requestType.compare("TRACE") == 0)
		result = TRACE;

	return result;
}

string HeaderToSend(const string& statusMessage)
{
	string response = statusMessage;
	response += "\nServer: Itamar-Shay Server\nContent-Type: text/html\nContent-length: ";
	return response;
}

string GetFileContent(string request)
{
	request = request.substr(request.find(' ') + 1, request.size());
	string fileName = request.substr(0, request.find(' '));
	if (fileName[0] == '/')
		fileName = fileName.substr(1, fileName.size());
	string content;
	if (fileName.size() > 1)
	{
		ifstream file;
		file.open(fileName);
		if (file.fail()) return "File not found";
		char tav = file.get();
		while (tav != EOF)
		{
			content.push_back(tav);
			tav = file.get();
		}
	}
	else content = "<!DOCTYPE HTML>\n<html><p>Itamar-Shay Server</p></html>";
	return content;
}

string writeToFile(string body, string fileName)
{
	if (fileName[0] == '/') fileName = fileName.substr(1, fileName.size());
	ofstream outputFile;
	outputFile.open(fileName);
	if (outputFile.is_open())
	{
		outputFile << body;
		outputFile.close();
		return "HTTP/1.1 201 Created";
	}
	else
	{
		return "HTTP/1.1 500 Internal Server Error";
	}
}

string deleteFile(string fileName)
{
	if (fileName[0] == '/')
		fileName = fileName.substr(1, fileName.size());

	FILE* file = fopen(fileName.c_str(), "r");
	if (file != nullptr)
	{
		fclose(file);
		if (remove(fileName.c_str()) == 0)
		{
			return "HTTP/1.1 200 OK";
		}
		else
		{
			return "HTTP/1.1 500 Internal Server Error";
		}
	}
	else
	{
		return "HTTP/1.1 404 Not Found";
	}
}


string GetAnswer(string request)
{
	string body = GetFileContent(request);
	string messageStatus = "HTTP/1.1 ";
	if (body.compare("File not found") == 0)
		messageStatus += "404 Not Found";
	else
		messageStatus += "200 OK";
	string httpHeader = HeaderToSend(messageStatus);
	char* MessageSize = new char[8];
	_itoa(body.size(), MessageSize, 10);
	httpHeader += MessageSize;
	delete[]MessageSize;
	return httpHeader + "\n\n" + body;
}

string PutAnswer(string request, string body)
{
	string requestSub = request.substr(request.find(' ') + 1, request.size());
	string fileName;
	if (requestSub.find(' ') == string::npos)
		fileName = requestSub;
	else
		fileName = requestSub.substr(0, requestSub.find(' '));

	string messageStatus = writeToFile(body, fileName);
	string header = HeaderToSend(messageStatus);
	header += "0\n\n";
	return header;
}

string DeleteAnswer(string request)
{
	string requestSub = request.substr(request.find(' ') + 1, request.size());
	string fileName = requestSub.substr(0, requestSub.find(' '));
	string statusMess = deleteFile(fileName);
	string header = HeaderToSend(statusMess);
	string body;
	if (statusMess.find("200") != string::npos)
		body = "<html><h1>URL deleted.</h1></html>";
	else
		body = "<html><h1>URL not deleted.</h1></html>";
	header.append(to_string(body.size()));
	return header + "\n\n" + body;
}

string OptionsAnswer()
{
	string returnMessage = HeaderToSend("HTTP/1.1 204 No Content");
	string methodsAllowed = "Access-Control-Allow-Methods: GET, PUT, OPTIONS, TRACE, HEAD, DELETE, Exit";
	returnMessage += "0";
	returnMessage += "\n" + methodsAllowed + "\n\n";
	return returnMessage;
}

string TraceAnswer(string request)
{
	string answer("HTTP/1.1 200 OK");
	answer += "\nServer: Itamar-Shay Server\nContent-Type: message/http";
	return answer;
}

string HeadAnswer(string request)
{
	string body = GetFileContent(request);
	string messageStatus = "HTTP/1.1 ";
	if (body.compare("File not found") == 0)
		messageStatus.append("404 Not Found");
	else
		messageStatus.append("200 OK");

	string header = HeaderToSend(messageStatus);
	char* MessageSize = new char[8];
	_itoa(body.size(), MessageSize, 10);
	header += MessageSize;
	delete[]MessageSize;
	return header;
}
