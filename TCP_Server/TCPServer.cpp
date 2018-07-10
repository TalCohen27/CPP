#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <fstream>

char * getFilePath(char *recvBuff);
char *getStatusCodeText(int statusCode);
char * getResponseHeader(int statusCode);
void Trace(char *filePath, char *sendBuff, char *recvBuff);
void Delete(char *filePath, char *sendBuff);
void Put(char *filePath, char *sendBuff, char *recvBuff);
void Head(char *filePath, char *sendBuff);
void Get(char *filePath, char *recvBuff, char *sendBuff);

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[128];
	int len;
	time_t latestTime;	// timestamp for last request
};

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN  = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int GET = 1;
const int HEAD = 2;
const int PUT = 3;
const int DELET = 4;
const int TRACE = 5;
const int STATUS_CODE_OK = 200;
const int STATUS_CODE_FILE_NOT_FOUND = 404;
const int INTERNAL_SERVER_ERROR = 500;
const char * HTTP_VERSION = "HTTP/1.1";
const double TIMEOUT = 90;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

struct SocketState sockets[MAX_SOCKETS]={0};
int socketsCount = 0;


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
	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
        cout<<"Time Server: Error at WSAStartup()\n";
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
        cout<<"Time Server: Error at socket(): "<<WSAGetLastError()<<endl;
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
    if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR *) &serverService, sizeof(serverService))) 
	{
		cout<<"Time Server: Error at bind(): "<<WSAGetLastError()<<endl;
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
		timeval timeout;
		timeout.tv_sec = 90;
		timeout.tv_usec = 0;

		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, &timeout);
		if (nfd == SOCKET_ERROR)
		{
			cout <<"Time Server: Error at select(): " << WSAGetLastError() << endl;
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
			// Checking that the connection duration hasn't passed the timeout threshold. 
			// If yes, we close the connection
			for (int i = 1; i < MAX_SOCKETS; i++) 
			{
				if ((sockets[i].recv != EMPTY) && ((int)(time(0) - sockets[i].latestTime) > TIMEOUT)) 
				{
					closesocket(sockets[i].id);
					removeSocket(i);
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
			sockets[i].latestTime = time(0);
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;

			 // Set the socket to be in non-blocking mode.
            unsigned long flag = 1;
            if (ioctlsocket(sockets[i].id, FIONBIO, &flag) != 0)
            {
                cout<<"Web Server: Error at ioctlsocket() #"<<i<<": "<<WSAGetLastError()<<endl;
            }

			return (true);
		}
	}
	return (false);
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
	cout << "Time Server: Client "<<inet_ntoa(from.sin_addr)<<":"<<ntohs(from.sin_port)<<" is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	/*unsigned long flag=1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout<<"Time Server: Error at ioctlsocket(): "<<WSAGetLastError()<<endl;
	}
*/
	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout<<"\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;
	sockets[index].latestTime = time(0);
	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len, 0);
	
	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
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
		//sockets[index].send = SEND;
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout<<"Time Server: Recieved: "<<bytesRecv<<" bytes of \""<<&sockets[index].buffer[len]<<"\" message.\n";
		
		sockets[index].len += bytesRecv;

		if (sockets[index].len > 0)
		{
			if (strncmp(sockets[index].buffer, "GET", 3) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = GET;
				return;
			}
			else if (strncmp(sockets[index].buffer, "HEAD", 4) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = HEAD;
				return;
			}
			else if (strncmp(sockets[index].buffer, "PUT", 3) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = PUT;
				return;
			}
			else if (strncmp(sockets[index].buffer, "DELETE", 6) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = DELET;

				return;
			}
			else if (strncmp(sockets[index].buffer, "TRACE", 5) == 0)
			{
				sockets[index].send  = SEND;
				sockets[index].sendSubType = TRACE;

				return;
			}
			else if (strncmp(sockets[index].buffer, "Exit", 4) == 0)
			{
				closesocket(msgSocket);
				removeSocket(index);
				return;
			}

		}
	}

}

void sendMessage(int index)
{
	int bytesSent = 0;
	char sendBuff[255];
	char * filePath = getFilePath(sockets[index].buffer);

	SOCKET msgSocket = sockets[index].id;

	if (sockets[index].sendSubType == GET)
	{
		Get(filePath, sockets[index].buffer, sendBuff);	// Answer client's GET request.
	}
	else if(sockets[index].sendSubType == HEAD)
	{
		Head(filePath, sendBuff);		// Answer client's HEAD request
	}
	else if(sockets[index].sendSubType == PUT)
	{
		Put(filePath, sendBuff, sockets[index].buffer);  // Answer client's PUT request
	}
	else if(sockets[index].sendSubType == DELET)
	{
		Delete(filePath, sendBuff);				// Answer client's DELET request
	}
	else if(sockets[index].sendSubType == TRACE)
	{
		Trace(filePath, sendBuff, sockets[index].buffer);		// Answer client's TRACE request
	}

	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Time Server: Error at send(): " << WSAGetLastError() << endl;	
		return;
	}

	cout<<"Time Server: Sent: "<<bytesSent<<"\\"<<strlen(sendBuff)<<" bytes of \""<<sendBuff<<"\" message.\n";	

	int currenLen = strlen(sockets[index].buffer);
    memcpy(sockets[index].buffer, &sockets[index].buffer[currenLen], sockets[index].len - currenLen);
    sockets[index].len -= currenLen;

	if(sockets[index].len >= 0)
	{
		sockets[index].send = IDLE;
	}
}

void Get(char *filePath, char *recvBuff, char *sendBuff)
{
	char *fileData = new char[0];
	int statusCode;

	fstream reqFile(filePath);
	if(reqFile == NULL)
	{
		statusCode = STATUS_CODE_FILE_NOT_FOUND;
		sprintf(sendBuff, "%s", getResponseHeader(statusCode));
	}
	else
	{
		statusCode = STATUS_CODE_OK;
		reqFile.seekg(0, reqFile.end);
	    int len = reqFile.tellg();
		fileData = new char[len];
		reqFile.seekg (0, reqFile.beg);
		reqFile.read(fileData, len);
		fileData[len] = '\0';
		sprintf(sendBuff, "%s\r\n%s", getResponseHeader(statusCode), fileData);
	}
	
	reqFile.close();
}

void Head(char *filePath, char *sendBuff)
{
	int statusCode;
	fstream reqFile(filePath);

	if(reqFile == NULL)
	{
		statusCode = STATUS_CODE_FILE_NOT_FOUND;
	}
	else
	{
		statusCode = STATUS_CODE_OK;
	}

	sprintf(sendBuff, "%s", getResponseHeader(statusCode));
}

void Put(char *filePath, char *sendBuff, char *recvBuff)
{
	fstream file(filePath);
	using std::ios_base;
    if(!file.is_open())
	{
		file.open(filePath, ios_base::in | ios_base::out | ios_base::trunc);
	}
		char * dataToFile = strstr(recvBuff, "\r\n\r\n");
		dataToFile += 4;
		int len = strlen(dataToFile);
		file.write(dataToFile, len); 
		strcpy(sendBuff, getResponseHeader(STATUS_CODE_OK));
	

	file.close();
}

void Delete(char *filePath, char *sendBuff)
{
	int statusCode = 0;
	fstream reqFile(filePath);
	if(reqFile == NULL)
	{
		statusCode = STATUS_CODE_FILE_NOT_FOUND;
	}
	else
	{
		if(remove(filePath) != 0)
		{
			statusCode = INTERNAL_SERVER_ERROR;
		}
		else
		{
			statusCode = STATUS_CODE_OK;
		}
	}

	sprintf(sendBuff, "%s", getResponseHeader(statusCode));
	
}

void Trace(char *filePath, char *sendBuff, char *recvBuff)
{
	sprintf(sendBuff, "%s\r\n%s", getResponseHeader(STATUS_CODE_OK), recvBuff); 
}

char * getResponseHeader(int statusCode)
{
	char *resHeader = new char[255];
	char *statusCodeText = "";

	statusCodeText = getStatusCodeText(statusCode);
	sprintf(resHeader, "%s %d %s", HTTP_VERSION, statusCode, statusCodeText);

	return resHeader;
}

char *getStatusCodeText(int statusCode)
{
	char *resStr;

	switch(statusCode)
	{
		case STATUS_CODE_OK: resStr = "OK";
			break;
		case STATUS_CODE_FILE_NOT_FOUND: resStr = "File not found";
			break;
		case INTERNAL_SERVER_ERROR: resStr = "Internal server error";
	}

	return resStr;
}

char * getFilePath(char *recvBuff)
{
	char *copy = new char[strlen(recvBuff)];
	strcpy(copy, recvBuff);
	char *result = strtok(copy, " ");
	result = strtok(NULL, " ");

	return result;
}

