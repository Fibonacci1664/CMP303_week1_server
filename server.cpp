/*	CMP303 TCP server example - by Henry Fortuna and Adam Sampson
	Updated by Gaz Robinson & Andrei Boiko

	A simple TCP server that waits for a connection.
	When a connection is made, the server sends "hello" to the client.
	It then repeats back anything it receives from the client.
	All the calls are blocking -- so this program only handles
	one connection at a time.
*/

#include<iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// The IP address for the server to listen on
#define SERVERIP "127.0.0.1"

// The TCP port number for the server to listen on
#define SERVERPORT 5555

// The message the server will send when the client connects
#define WELCOME "You're connected!"

// The (fixed) size of message that we send between the two programs
#define MESSAGESIZE 32

// Prototypes
void die(const char *message);
std::vector<char> modMessage(char rcvBuff[MESSAGESIZE]);
bool talkToClient(SOCKET clientSocket);

int main()
{
	printf("Echo Server\n");

	// Initialise the WinSock library -- we want version 2.2.
	WSADATA w;

	// If successful, the WSAStartup function returns zero.
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
	int error = WSAStartup(0x0202, &w);

	if (error != 0)
	{
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		die("WSAStartup failed");
	}
	if (w.wVersion != 0x0202)
	{
		/* Tell the user that we could not find a correct version of */
		/* WinSock DLL.												 */
		die("Wrong WinSock version");
	}

	// Create a TCP socket that we'll use to listen for connections.
	// AF_INET = internet Family Address(AF) for IPv4
	// AF_INET6 = internet Family Address(AF) for IPv6
	// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-socket
	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	// FIXME: we should test for error here
	// ############# FIXED #############
	if (serverSocket == INVALID_SOCKET)
	{
		die("socket function failed with error");
	}
	else
	{
		wprintf(L"socket function succeeded\n");
	}

	// Fill out a sockaddr_in structure to describe the address we'll listen on.
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(SERVERIP);

	// Test to make sure the IP address is valid
	// ############# FIXED #############
	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
	{
		die("not a legitimate server IP address");
	}

	// htons converts the port number to network byte order (big-endian).
	serverAddr.sin_port = htons(SERVERPORT);

	// Bind the server socket to that address.
	// listen() will fail if bind() is not called.
	int bindAddr = bind(serverSocket, (const sockaddr*)&serverAddr, sizeof(serverAddr));

	if (bindAddr != 0)
	{
		die("server bind failed");
	}

	// ntohs does the opposite of htons.
	char* IPv4addr_as_str = inet_ntoa(serverAddr.sin_addr);

	if (IPv4addr_as_str == nullptr)
	{
		die("error converting IPv4 network address to ASCII string");
	}

	printf("Server socket bound to address %s, port %d\n", IPv4addr_as_str, ntohs(serverAddr.sin_port));

	// Make the socket listen for connections.
	// accept() will fail if listen() is not called.
	int listener = listen(serverSocket, 1);

	if (listener != 0)
	{
		die("listen failed");
	}

	printf("Server socket listening\n");

	while (true)
	{
		printf("Waiting for a connection...\n");

		// Accept a new connection to the server socket.
		// This gives us back a new socket connected to the client, and
		// also fills in an address structure with the client's address.
		sockaddr_in clientAddr;
		int addrSize = sizeof(clientAddr);
		SOCKET clientSocket = accept(serverSocket, (sockaddr *) &clientAddr, &addrSize);

		if(clientSocket == INVALID_SOCKET)
		{
			//die("accept failed");
			// FIXME: in a real server, we wouldn't want the server to crash if
			// it failed to accept a connection -- recover more effectively!

			// print error msg
			printf("\n#### Accepting client failed! ####\n\n");

			// Just try again
			continue;

			// Or run this code if you want to exit upon fail
			// close server socket, then break from loop
			// which closes server socket and cleans up.
			/*printf("Closing server socket...\n");
			break;*/
		}

		printf("Client has connected from IP address %s, port %d!\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

		bool flag = talkToClient(clientSocket);

		if (flag)
		{
			break;
		}
	}

	// We will get here, when we do then we want to clean up...
	printf("Closing connection\n");
	closesocket(serverSocket);
	WSACleanup();
	printf("Quitting...\n");
	return 0;
}

bool talkToClient(SOCKET clientSocket)
{
	// We'll use this array to hold the messages we exchange with the client.
	char rcvBuffer[MESSAGESIZE];

	// Fill the buffer with - characters to start with.
	memset(rcvBuffer, '-', MESSAGESIZE);

	// Send a welcome message to the client.
	memcpy(rcvBuffer, WELCOME, strlen(WELCOME));
	int welcomeMsg = send(clientSocket, rcvBuffer, MESSAGESIZE, 0);

	// FIXME: check for errors from send
	// ############# FIXED #############
	if (welcomeMsg == SOCKET_ERROR)
	{
		die("welcome message send error");
	}

	bool flag = false;

	while (true)
	{
		// Receive as much data from the client as will fit in the buffer.
		int count = recv(clientSocket, rcvBuffer, MESSAGESIZE, 0);

		// FIXME: check for errors from recv
		// ############# FIXED #############
		if (count == SOCKET_ERROR)
		{
			die("message recv error");
		}

		if (count <= 0)
		{
			printf("Client closed connection\n");
			flag = true;
			break;
		}

		if (count != MESSAGESIZE)
		{
			die("Got strange-sized message from client");
		}

		if (memcmp(rcvBuffer, "quit", 4) == 0)
		{
			printf("Client asked to quit\n");
			flag = true;
			break;
		}

		// (Note that recv will not write a \0 at the end of the message it's
		// received -- so we can't just use it as a C-style string directly
		// without writing the \0 ourself.)

		printf("Received %d bytes from the client: '", count);
		fwrite(rcvBuffer, 1, count, stdout);

		std::vector<char> rcvBuffVec = modMessage(rcvBuffer);

		// Copy from the modified vec back into the rcvBuffer object.
		for (int i = 0; i < rcvBuffVec.size(); ++i)
		{
			rcvBuffer[i] = rcvBuffVec[i];
		}

		printf("'\n");

		// Send the same data back to the client.
		int reply = send(clientSocket, rcvBuffer, MESSAGESIZE, 0);

		// FIXME: check for errors from send
		// ############# FIXED #############
		if (reply == SOCKET_ERROR)
		{
			die("echo send error, server side");
		}
	}

	if (flag)
	{
		return true;
	}

	return false;
}

std::vector<char> modMessage(char rcvBuff[MESSAGESIZE])
{
	std::vector<char> rcvBuffCopy;

	// Copy rcvBuff array to vec
	for(int i = 0; i < MESSAGESIZE; ++i)
	{
		rcvBuffCopy.push_back(rcvBuff[i]);
	}

	// Modify vec
	for (int i = 0; i < rcvBuffCopy.size(); ++i)
	{
		rcvBuffCopy[i] = std::toupper(rcvBuffCopy[i]);
	}

	return rcvBuffCopy;
}

// Print an error message and exit.
void die(const char *message) {
	fprintf(stderr, "Error: %s (WSAGetLastError() = %d)\n", message, WSAGetLastError());

#ifdef _DEBUG
	// Debug build -- drop the program into the debugger.
	abort();
#else
	exit(1);
#endif
}