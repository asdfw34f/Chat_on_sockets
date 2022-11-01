
#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX 256

WSADATA wsaData;
int iResult, iRes1, iRes2;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket[2] = { INVALID_SOCKET, INVALID_SOCKET };

struct addrinfo* result = NULL;
struct addrinfo hints;

int iSendResult;
char recvbuf[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

HANDLE p, p1;
DWORD IDPthread[2];


DWORD WINAPI Client0(LPVOID Q)
{
    iRes1 = recv(ClientSocket[0], recvbuf, recvbuflen, 0);
    if (iRes1 > 0)
    {
        //printf("Bytes received: %d\n", iRes1);
        recvbuf[iRes1] = 0;
        printf("Client message: %s\n", recvbuf);

        // Send an initial buffer
        int i = send(ClientSocket[1], recvbuf, iRes1, 0);
        if (i == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket[0]);
            WSACleanup();
            return 1;
        }
    }
    else if (iRes1 == 0)
        printf("Connection closing...\n");
    else {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket[0]);
        WSACleanup();
        return 1;
    }
    return 0;
}


DWORD WINAPI Client1(LPVOID Q)
{
    //client 2
    iRes2 = recv(ClientSocket[1], recvbuf, recvbuflen, 0);
    if (iRes2 > 0)
    {
        //printf("Bytes received: %d\n", iRes2);
        recvbuf[iRes2] = 0;
        printf("Client message: %s\n", recvbuf);

        // Send an initial buffer
        int i = send(ClientSocket[0], recvbuf, iRes2, 0);
        if (i == SOCKET_ERROR) {
            printf("send failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket[1]);
            WSACleanup();
            return 1;
        }
    }
    else if (iRes2 == 0)
        printf("Connection closing...\n");
    else {
        printf("recv failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket[1]);
        WSACleanup();
        return 1;
    }

    if (strncmp(recvbuf, "bye", strlen("bye")) == 0) {
        iRes1 = 0;
        iRes2 = 0;
    }

    return 0;
}

int __cdecl main(void)
{
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    bool isRunning = true;
    while (isRunning) {


            // Accept a client socket
            ClientSocket[0] = accept(ListenSocket, NULL, NULL);
        if (ClientSocket[0] == INVALID_SOCKET) {
            printf("accept failed with error ClientSocket 1: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        ClientSocket[1] = accept(ListenSocket, NULL, NULL);
        if (ClientSocket[1] == INVALID_SOCKET) {
            printf("accept failed with error ClientSocket 2 : %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            //return 1;
        }

        // Receive until the peer shuts down the connection
        do {

            p = CreateThread(NULL, 0, Client0, 0, 0, &IDPthread[0]);
            WaitForSingleObject(p, INFINITE);
            p1 = CreateThread(NULL, 0, Client1, 0, 0, &IDPthread[1]);
            WaitForSingleObject(p1, INFINITE);

        } while (iRes1 > 0 || iRes1 > 0);

        // shutdown the connection since we're done
        iResult = shutdown(ClientSocket[0], SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket[0]);
            WSACleanup();
            return 1;
        }

        iResult = shutdown(ClientSocket[1], SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket[1]);
            WSACleanup();
            return 1;
        }

        // cleanup
        closesocket(ClientSocket[0]);
        closesocket(ClientSocket[1]);
        CloseHandle(p);
        CloseHandle(p1);
    }

    // No longer need server socket
    closesocket(ListenSocket);
    WSACleanup();
    return 0;
}