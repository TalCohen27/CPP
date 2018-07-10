#include <iostream>
using namespace std;
// Don't forget to include "ws2_32.lib" in the library list.
#include <winsock2.h> 
#include <string.h>
#include <sstream>
#include<windows.h>

#define TIME_PORT	27015
const char initiChoice = '2';
const char quitChoice = '1';
const int loopCounter = 99;

void DelayEstimation(SOCKET connSocket, char *sendBuff, char *recvBuff, sockaddr_in server, int bytesSent, int bytesRecv);
void MeasureRTT(SOCKET connSocket, char *sendBuff, char *recvBuff, sockaddr_in server, int bytesSent, int bytesRecv);
void VariousTimeRequests(SOCKET connSocket, char *sendBuff, char *recvBuff, sockaddr_in server, int bytesSent, int bytesRecv);

void main() 
{
	char choice = initiChoice;
    // Initialize Winsock (Windows Sockets).

	WSAData wsaData; 
	if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
	{
		cout<<"Time Client: Error at WSAStartup()\n";
	}

	// Client side:
	// Create a socket and connect to an internet address.
    SOCKET connSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == connSocket)
	{
        cout<<"Time Client: Error at socket(): "<<WSAGetLastError()<<endl;
        WSACleanup();
        return;
	}

	// For a client to communicate on a network, it must connect to a server.
    
	// Need to assemble the required data for connection in sockaddr structure.

    // Create a sockaddr_in object called server. 
	sockaddr_in server;
    server.sin_family = AF_INET; 
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(TIME_PORT);

	// loop waiting for client's choice till quit
	while(choice != quitChoice)
	{
		// Send and receive data.
		int bytesSent = 0;
		int bytesRecv = 0;
		char sendBuff[255];
		char recvBuff[255];

		// menu options
		cout<<"Choose an option (to quit press " <<quitChoice <<"): "<<endl;
		cout<<"a. What's the time?"<<endl;
		cout<<"b. What's the time without date?"<<endl;
		cout<<"c. What's the time since epoch?"<<endl;
		cout<<"d. Client to server Delay Estimation"<<endl;
		cout<<"e. RTT"<<endl;
		cout<<"f. What's the year?"<<endl;
		cout<<"g. What's the day and month?"<<endl;
		cout<<"h. What's the number of seconds since beg. of month?"<<endl;

		cin>>choice;
		// copying the request into sendBuff
		switch(choice)
		{
			case 'a':   strcpy(sendBuff , "What's the time?");
				break;
			case 'b':    strcpy(sendBuff, "What's the time without date?");
				break;
			case 'c': strcpy(sendBuff, "What's the time since epoch?");
				break;
			case 'd': strcpy(sendBuff, "Delay Estimation");
				break;
			case 'e': strcpy(sendBuff, "RTT");
				break;
			case 'f': strcpy(sendBuff, "What's the year?");
				break;
			case 'g': strcpy(sendBuff, "What's the day and month?");
				break;
			case 'h': strcpy(sendBuff, "What's the number of seconds since beg. of month?");
				break;
		}

		WSAData wsaData; 
		if (NO_ERROR != WSAStartup(MAKEWORD(2,2), &wsaData))
		{
			cout<<"Time Client: Error at WSAStartup()\n";
		}

		// Client side:
		// Create a socket and connect to an internet address.
		SOCKET connSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (INVALID_SOCKET == connSocket)
		{
			cout<<"Time Client: Error at socket(): "<<WSAGetLastError()<<endl;
			WSACleanup();
			return;
		}

		// For a client to communicate on a network, it must connect to a server.
    
		// Need to assemble the required data for connection in sockaddr structure.

		// Create a sockaddr_in object called server. 
		sockaddr_in server;
		server.sin_family = AF_INET; 
		server.sin_addr.s_addr = inet_addr("127.0.0.1");
		server.sin_port = htons(TIME_PORT);


		// Asks the server what's the currnet time.
		// The send function sends data on a connected socket.
		// The buffer to be sent and its size are needed.
		// The fourth argument is an idicator specifying the way in which the call is made (0 for default).
		// The two last arguments hold the details of the server to communicate with. 
		// NOTE: the last argument should always be the actual size of the client's data-structure (i.e. sizeof(sockaddr)).

		// checking for chosen request and sending it accordingly

		// for delay estimation we need to send 100 requests
		if(strcmp(sendBuff, "Delay Estimation") == 0)
		{
			DelayEstimation(connSocket, sendBuff, recvBuff, server, bytesSent, bytesRecv);
	
		} // same for RTT
		else if(strcmp(sendBuff, "RTT") == 0)
		{
			MeasureRTT(connSocket, sendBuff, recvBuff, server, bytesSent, bytesRecv);
		}
		else // all the others
		{
			VariousTimeRequests(connSocket, sendBuff, recvBuff, server, bytesSent, bytesRecv);
		}
	
		// Closing connections and Winsock.
		cout<<"Time Client: Closing Connection.\n";
		closesocket(connSocket);
	}
}

void DelayEstimation(SOCKET connSocket, char *sendBuff, char *recvBuff, sockaddr_in server, int bytesSent, int bytesRecv)
{
	int firstTimestamp, secondTimestamp, sumOfDiff = 0;
	stringstream timestamp;

		for(int i = 0; i <= loopCounter; i++) // sending 100 estimation requests
		{
			bytesSent = sendto(connSocket, sendBuff, (int)strlen(sendBuff), 0, (const sockaddr *)&server, sizeof(server));

			if (SOCKET_ERROR == bytesSent)
			{
				cout<<"Time Client: Error at sendto(): "<<WSAGetLastError()<<endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
		}
		
		bytesRecv = recv(connSocket, recvBuff, 255, 0);  // getting the first response
		if (SOCKET_ERROR == bytesRecv)
		{
			cout<<"Time Client: Error at recv(): "<<WSAGetLastError()<<endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
		
		recvBuff[bytesRecv] = '\0'; // add the null-terminating to make it a string
		cout<<"Time Client: Recieved: "<<bytesRecv<<" bytes of \""<<recvBuff<<"\" message.\n";
		timestamp << recvBuff;			
		timestamp >> firstTimestamp;
		
		//cout<<"Time Client: Recieved: "<<bytesRecv<<" bytes of \""<<recvBuff<<"\" message.\n";

	for(int i = 0; i < loopCounter; i++)		// getting the remaining 99 responses
	{
		bytesRecv = recv(connSocket, recvBuff, 255, 0);
		if (SOCKET_ERROR == bytesRecv)
		{
			cout<<"Time Client: Error at recv(): "<<WSAGetLastError()<<endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
		
		cout<<"Time Client: Recieved: "<<bytesRecv<<" bytes of \""<<recvBuff<<"\" message.\n";
		timestamp.clear();
		timestamp << recvBuff;
		timestamp >> secondTimestamp;
		sumOfDiff += secondTimestamp - firstTimestamp;  // adding the difference between two adjacent responses to the sum
		firstTimestamp = secondTimestamp;				// updating the firstTimestamp
		recvBuff[bytesRecv]='\0'; // add the null-terminating to make it a string	
	}

	cout<<"The estimation is: "<<sumOfDiff/loopCounter;		// this is the final average
}

void MeasureRTT(SOCKET connSocket, char *sendBuff, char *recvBuff, sockaddr_in server, int bytesSent, int bytesRecv)
{
	int sentTimestamp, recvTimestamp, sumOfDiff = 0;
	
	    for(int i = 0; i <= 99; i++)		// sending requests
		{
			sentTimestamp = GetTickCount();	// getting the timestamp before for 'sent' time
			bytesSent = sendto(connSocket, sendBuff, (int)strlen(sendBuff), 0, (const sockaddr *)&server, sizeof(server));

			if (SOCKET_ERROR == bytesSent)
			{
				cout<<"Time Client: Error at sendto(): "<<WSAGetLastError()<<endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}

			bytesRecv = recv(connSocket, recvBuff, 255, 0);
			if (SOCKET_ERROR == bytesRecv)
			{
				cout<<"Time Client: Error at recv(): "<<WSAGetLastError()<<endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
			
			recvTimestamp = GetTickCount(); // getting timestamp for 'recv' time
			recvBuff[bytesRecv] = '\0'; // add the null-terminating to make it a string
			cout<<"Time Client: Recieved: "<<bytesRecv<<" bytes of \""<<recvBuff<<"\" message.\n";
			sumOfDiff += recvTimestamp - sentTimestamp;   // adding the difference to the sum
			// recvBuff[bytesRecv]='\0'; //add the null-terminating to make it a string
		}
		
		cout<<"The average RTT is: "<<sumOfDiff/(loopCounter + 1); 
}

void VariousTimeRequests(SOCKET connSocket, char *sendBuff, char *recvBuff, sockaddr_in server, int bytesSent, int bytesRecv)
{
	bytesSent = sendto(connSocket, sendBuff, (int)strlen(sendBuff), 0, (const sockaddr *)&server, sizeof(server));
	if (SOCKET_ERROR == bytesSent)
	{
		cout<<"Time Client: Error at sendto(): "<<WSAGetLastError()<<endl;
		closesocket(connSocket);
		WSACleanup();
		return;
	}
	cout<<"Time Client: Sent: "<<bytesSent<<"/"<<strlen(sendBuff)<<" bytes of \""<<sendBuff<<"\" message.\n";
	
	// Gets the server's answer using simple recieve (no need to hold the server's address).
	bytesRecv = recv(connSocket, recvBuff, 255, 0);
	if(SOCKET_ERROR == bytesRecv)
	{
		cout<<"Time Client: Error at recv(): "<<WSAGetLastError()<<endl;
		closesocket(connSocket);
		WSACleanup();
		return;
	}

	recvBuff[bytesRecv]='\0'; //add the null-terminating to make it a string
	cout<<"Time Client: Recieved: "<<bytesRecv<<" bytes of \""<<recvBuff<<"\" message.\n";
}