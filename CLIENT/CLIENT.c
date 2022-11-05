#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <synchapi.h>
#include <time.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;

struct addrinfo* result = NULL, * ptr, hints;
int iResult;

char recvbuf[DEFAULT_BUFLEN] = { 0 };
int recvbuflen = DEFAULT_BUFLEN;
char buffer[MAX_PATH] = { 0 };

int __cdecl main(int argc, char** argv)
{
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) 
    {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, 
            ptr->ai_socktype,
            ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, 
            ptr->ai_addr, 
            (int)ptr->ai_addrlen);

        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

        BOOL isRunning = TRUE;
    while (isRunning) 
    {
        printf("Enter message:\n");
        scanf_s("%s", buffer, sizeof(buffer));

        // Send an initial buffer
        iResult = send(ConnectSocket, 
            buffer, 
            (int)strlen(buffer), 0);

        if (iResult == SOCKET_ERROR)
            isRunning = FALSE;
        if (strncmp(buffer, "bye", strlen("bye")) == 0)
            isRunning = FALSE;

        memset(recvbuf, 0, sizeof(recvbuf));
        iResult = recv(ConnectSocket,
            recvbuf, recvbuflen, 0);
        printf("New message: %s\n", recvbuf);
        if (iResult > 0)
            printf("New message: %s\n", recvbuf);
        else if (iResult == 0) 
            isRunning = FALSE;
        //else {
        //    printf("recv failed with error: %d\n",
        //        WSAGetLastError());
        //    isRunning = false;
        //}
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    freeaddrinfo(result);
    printf("Client disconnected!\n");

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}