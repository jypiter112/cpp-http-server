#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <sstream>
#pragma comment(lib, "Ws2_32.lib")

using std::vector, std::string;
SOCKET* client_p;
int SendIndex(SOCKET, std::string); 

/*
    Basic GET req support
    to do POST req support
*/

vector<string> ParsePostRequest(char* req){
    vector<string> output_r;
    string r(req);
    r = r.substr(r.find("\n\n") + 2);
    std::stringstream ss(r);
    string token;
    while(std::getline(ss, token, '&')){
        output_r.push_back(token);
    }
    return output_r;
}

std::string GetPathFromReq(char* req){
    if(strlen(req) < 15) return "";
    std::string r(req, 25);
    std::string path;
    // Parse request types
    /*
        Supported : GET
    */
    if(r.substr(0, 3) == "GET"){
        int i = r.substr(4).find(" ");
        if(i > 0){
            path = r.substr(4, i);
        } else {
            std::cout << "Invalid path\n";
            return "";
        }
        return path.substr(1);
    }
    if(r.substr(0, 4) == "POST" || r.substr(0, 4) == "post"){
        // Doesnt work At the moment
        std::cout << "POST" << req << "END OF POST\n";
        vector<string> post_data = ParsePostRequest(req);
        for(auto i : post_data){
            std::cout << i.substr(i.find('=') + 1) << std::endl;
        }
    }
    return "";
}
int SendIndex(SOCKET client, std::string filename) {
    /*
        Insecure!!! Can send prettymuch any file back
        To Be Fixed!
    */
    std::cout << "[Server] Sending res:\n";
    std::string response;
    std::ifstream file(filename);
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
    // Parse filetype requested
    /*
        Supported: css, js
        everything else -> text/html
    */
    std::string filetype(filename.substr(filename.find('.') + 1, filename.length()));
    std::string restype = "text/html";
    if(filetype == "css") restype = "stylesheet";
    if(filetype == "js") restype = "text/javascript";
    // --
    // Write response to memory
    response = "HTTP/1.0 200 OK\n";
    response += "Server: LeanAndMean\n";
    response += "Content-Length: " + std::to_string(body.length()) + "\n";
    response += "Content-Type: " + restype + "; charset=UTF-8\n\n";
    response += body;

    // Send response
    const char* final_res = response.c_str();
    send(client, final_res, strlen(final_res), 0);
    std::cout << final_res << std::endl;
    std::cout << "[Server] End of res ----\n";
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
        client_p = &client;
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
                // printf("Client > \n%s\n", request);
                std::string path = GetPathFromReq(request);
                if(!path.empty()) SendIndex(client, path);
                else std::cout << "[Client] Request:" << request << "[Client] End of request ---\n";
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
