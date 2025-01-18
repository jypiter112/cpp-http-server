#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#pragma comment(lib, "Ws2_32.lib")

int SendIndex(SOCKET client) {
    // Make response
    std::cout << "Sending res:\n";
    std::string response;
    std::ifstream file("C:\\Users\\joona\\source\\repos\\server\\server\\index.html");
    if (!file.is_open()) {
        std::cerr << "Couldnt open file\n";
        return 1;
    }

    std::string body = "";
    std::string line;
    while (std::getline(file, line)) {
        if(line.size() > 0) body += line + "\n";
    }
    file.close();
    // --
    // Write response to memory
    response = "HTTP/1.0 200 OK\n";
    response += "Server: LeanAndMean\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\n";
    response += "Content-Type: text/html; charset=UTF-8\n\n";
    response += body;

    // Send response
    const char* final_res = response.c_str();
    send(client, final_res, strlen(final_res), 0);
    std::cout << final_res << std::endl;
    return 0;
}

int main(int argc, char** argv) {
    // Open server
    WSADATA wsaData;
    struct sockaddr_in addr;
    SOCKET client; 
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d", WSAGetLastError());
    }
    else {
        printf("WsaStartup done.\n");
    }
    // Create socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("Socket creation failed: %d\n", WSAGetLastError());
        return 1;
    }
    // Try bind
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY); // bind to 0.0.0.0
    addr.sin_port = htons(8080);
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("Bind failed: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }
    printf("Bind successful.\n");

    // Listen for incoming connections
    while (1) {
        if (listen(sock, 10) == SOCKET_ERROR) {
            printf("Listen failed: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        printf("Listening for connections...");

        // Accept a client connection
        client = accept(sock, NULL, NULL);
        if (client == INVALID_SOCKET) {
            printf("Accept failed: %d\n", WSAGetLastError());
            closesocket(sock);
            WSACleanup();
            return 1;
        }
        printf("Client accepted.\n");

        char request[256] = { 0 };
        // Actively wait for data
        while (1) {
            // Receive data
            int recvResult = recv(client, request, sizeof(request) - 1, 0);
            if (recvResult > 0) {
                request[recvResult] = '\0'; // Null terminate
                printf("Client > \n%s\n", request);
                // Check if its a GET request
                std::string fb(request, 3);
                if (fb == "GET") SendIndex(client);
            }
            else if (recvResult == 0) {
                printf("Connection closed by client.\n");
                break;
            }
            else {
                printf("Receive failed: %d\n", WSAGetLastError());
                break;
            }
        }
    }
    // Cleanup
    closesocket(client);
    closesocket(sock);
    WSACleanup();
    return 0;
}
