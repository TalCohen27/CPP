#include <iostream>
using namespace std;
// Don't forget to include "Ws2_32.lib" in the library list.
#include <winsock2.h> 
#include <string.h>
#include <iomanip>

const int WEB_PORT = 27015;


void main() 
{

    // Initialize Winsock (Windows Sockets).

	WSAData wsaData; 
	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
		cout << "Time Client: Error at WSAStartup()\n";
		return;
	}

	// Client side:
	// Create a socket and connect to an internet address.

    SOCKET connSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == connSocket)
	{
        cout << "Time Client: Error at socket(): " << WSAGetLastError() << endl;
        WSACleanup();
        return;
	}

	//
	// Binding the created socket to the IP address and port representing "us".
	//
	//Note: for clients, if the socket is left unbound, the first call for connect() will 
	//bind the socket with arbitrary properties (IP and Port)
	
	//If specific values are required, you can use the following code:

	//struct sockaddr_in me;
	//me.sin_family = AF_INET;
    //me.sin_addr.s_addr = INADDR_ANY;
	//me.sin_port = 0;
	//
	// Binding the created socket to the IP address and port representing "us".
	//
	//if (bind(connSocket, (struct sockaddr*)&me, sizeof(me)) == SOCKET_ERROR)
	//{
	//	cout<<"Time Server: Error at bind(): "<<WSAGetLastError()<<endl;
    //  closesocket(connSocket);
	//	WSACleanup();
    //  return;
    //}

	// For a client to communicate on a network, it must connect to a server.    
	// Need to assemble the required data for connection in sockaddr structure.
    // Create a sockaddr_in object called server. 
	sockaddr_in server;
    server.sin_family = AF_INET; 
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(WEB_PORT);

	// Connect to server.

    // The connect function establishes a connection to a specified network 
	// address. The function uses the socket handler, the sockaddr structure 
	// (which defines properties of the desired connection) and the length of 
	// the sockaddr structure (in bytes).
	if (SOCKET_ERROR == connect(connSocket, (SOCKADDR *) &server, sizeof(server)))
	{
		cout << "Time Client: Error at connect(): " << WSAGetLastError() << endl;
        closesocket(connSocket);
		WSACleanup();
        return;
    }

    // Send and receive data.

    int bytesSent = 0;
    int bytesRecv = 0;
    char sendBuff[255];
    char recvBuff[255];
	int option = 0;

	while(option != 6)
	{
		cout<<"\nPlease choose an option:\n";
		cout<<"1 - GET\n";
		cout<<"2 - HEAD\n";
		cout<<"3 - PUT\n";
		cout<<"4 - DELETE\n";
		cout<<"5 - TRACE\n";
		cout<<"6 = Exit\n";
		cout<<">> ";

		cin>>option;

		if(option == 1)
			strcpy(sendBuff, "GET Hello.html HTTP/1.1\r\nHost: 127.0.0.1\r\n");
		else if(option == 2)
			strcpy(sendBuff, "HEAD Hello.html HTTP/1.1\r\nHost: 127.0.0.1\r\n");
		else if(option == 3)
			strcpy(sendBuff, "PUT Hello.html HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\nWorld!");
		else if(option == 4)
			strcpy(sendBuff, "DELETE Hello.html HTTP/1.1\r\nHost: 127.0.0.1\r\n");
		else if(option == 5)
			strcpy(sendBuff, "TRACE Hello.html HTTP/1.1\r\nHost: 127.0.0.1\r\n");
		else if(option == 6)
			strcpy(sendBuff, "Exit");
		
		// The send function sends data on a connected socket.
		// The buffer to be sent and its size are needed.
		// The last argument is an idicator specifying the way 
		// in which the call is made (0 for default).
		bytesSent = send(connSocket, sendBuff, (int)strlen(sendBuff), 0);
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "Time Client: Error at send(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}	
		cout<<"Time Client: Sent: "<<bytesSent<<"/"<<strlen(sendBuff)<<" bytes of \""<<sendBuff<<"\" message.\n";

		// Gets the server's answer for options 1 and 2.
		if(option == 1 || option == 2 || option == 3 || option == 4 || option == 5)
		{
			bytesRecv = recv(connSocket, recvBuff, 255, 0);
			if (SOCKET_ERROR == bytesRecv)
			{
				cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
			if (bytesRecv == 0)
			{
				cout<<"Server closed the connection\n";
				return;
			}
			
			recvBuff[bytesRecv]='\0'; //add the null-terminating to make it a string
			cout<<"Time Client: Recieved: "<<bytesRecv<<" bytes of \""<<recvBuff<<"\" message.\n";
		}
		else if(option == 6)
		{
			// Closing connections and Winsock.
			cout << "Time Client: Closing Connection.\n";
			closesocket(connSocket);
			WSACleanup();
		}
	}
}