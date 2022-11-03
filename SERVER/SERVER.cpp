#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX 256

WSADATA wsaData;
int iResult, iResClient;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

struct addrinfo* result = NULL;
struct addrinfo hints;

int iSendResult;

char MY_message[DEFAULT_BUFLEN], NEW_message[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

HANDLE ThreadClient, object, Thread_get, Thread_send;
DWORD IDthread_get, IDthread_send, IDPthreadClient;

DWORD WINAPI get_message(LPVOID lp)
{
    iResult = recv(ClientSocket, NEW_message, recvbuflen, 0);

    if (iResult > 0)
    {
        //printf("Bytes received: %d\n", iResult);
        printf("New message: %s\n", NEW_message);
    }
    else if (iResult == 0)
    {
        printf("Connection closed\n");
        ReleaseMutex(object);
    }
    else
    {
        printf("recv failed with error: %d\n", WSAGetLastError());
        ReleaseMutex(object);
    }

    memset(NEW_message, 0, sizeof(NEW_message));
    return 0;
}

DWORD WINAPI send_message(LPVOID lp)
{
    printf("Enter message:\n");
    scanf_s("%s", MY_message, sizeof(MY_message));

    // Send an initial MY_message
    iResult = send(ClientSocket, MY_message, (int)strlen(MY_message), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        ReleaseMutex(object);
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}


DWORD WINAPI Clients(LPVOID Q)
{
    Thread_get = CreateThread(NULL, 0, get_message, 0, 0, &IDPthreadClient);
    Thread_send = CreateThread(NULL, 0, send_message, 0, 0, &IDPthreadClient);

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
    object = CreateMutex(0, false, 0);

    bool isRunning = true;
    while (isRunning) {
        // Accept a client socket
        ClientSocket = accept(ListenSocket, NULL, NULL);
        if (ClientSocket == INVALID_SOCKET) {
            printf("accept failed with error ClientSocket 1: %d\n", WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }

        // Receive until the peer shuts down the connection
        do 
        {
            ThreadClient = CreateThread(NULL, 0, Clients, 0, 0, &IDPthreadClient);
        } while (iResClient > 0 || iResClient > 0);
        WaitForSingleObject(ThreadClient, INFINITE);

        // shutdown the connection since we're done
        iResult = shutdown(ClientSocket, SD_SEND);
        if (iResult == SOCKET_ERROR) {
            printf("shutdown failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
        // cleanup
        closesocket(ClientSocket);
    }
    
    // No longer need server socket
    closesocket(ListenSocket);
    WSACleanup();
    CloseHandle(ThreadClient);
    return 0;
}