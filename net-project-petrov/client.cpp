#include <iostream>
#include <string>

#ifdef _WIN32
    #define _WIN32_WINNT 0x0601
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET SocketType;
    #define INVALID_SOCK INVALID_SOCKET
    #define CLOSE_SOCKET(s) closesocket(s)
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    typedef int SocketType;
    #define INVALID_SOCK -1
    #define CLOSE_SOCKET(s) close(s)
#endif

using namespace std;

const char* SERVER_IP = "127.0.0.1";
const int PORT = 9000;

bool sendLine(SocketType sock, const string& line) {
    string data = line + "\n";
    size_t totalSent = 0;
    while (totalSent < data.size()) {
        int sent = send(sock, data.c_str() + totalSent, static_cast<int>(data.size() - totalSent), 0);
        if (sent <= 0) return false;
        totalSent += sent;
    }
    return true;
}

bool recvLine(SocketType sock, string& line) {
    line.clear();
    char ch;
    while (true) {
        int received = recv(sock, &ch, 1, 0);
        if (received <= 0) return false;
        if (ch == '\n') break;
        if (ch != '\r') line += ch;
    }
    return true;
}

int main() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed\n";
        return 1;
    }
#endif

    SocketType sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCK) {
        cerr << "Socket creation failed\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serverAddr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        cerr << "Connection failed. Make sure server is running on port " << PORT << ".\n";
        CLOSE_SOCKET(sock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    string response;
    if (recvLine(sock, response)) {
        cout << "Server: " << response << "\n";
    }

    string command;
    while (true) {
        cout << "> ";
        if (!getline(cin, command)) break;
        if (command.empty()) continue;

        if (!sendLine(sock, command)) {
            cerr << "Failed to send command.\n";
            break;
        }

        if (!recvLine(sock, response)) {
            cerr << "Server disconnected.\n";
            break;
        }

        cout << "Server: " << response << "\n";
        if (response == "BYE") break;
    }

    CLOSE_SOCKET(sock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
