#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <cctype>

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

const int PORT = 9000;

string toUpperStr(string s) {
    for (size_t i = 0; i < s.size(); ++i) {
        s[i] = static_cast<char>(toupper(static_cast<unsigned char>(s[i])));
    }
    return s;
}

string trimStr(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ============================================================
// ЗОНА СТУДЕНТА (Вариант 6: Список дел — Уровень 3)
// ============================================================

struct Task {
    string text;
    bool done;
    bool highPriority;
};

class TodoList {
private:
    vector<Task> tasks;
    const string filename = "todo.txt";

    void saveToFile() {
        ofstream out(filename.c_str());
        if (!out.is_open()) return;
        for (size_t i = 0; i < tasks.size(); ++i) {
            out << (tasks[i].highPriority ? 1 : 0) << "|"
                << (tasks[i].done ? 1 : 0) << "|"
                << tasks[i].text << "\n";
        }
    }

    void loadFromFile() {
        tasks.clear();
        ifstream in(filename.c_str());
        if (!in.is_open()) return;
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            size_t p1 = line.find('|');
            if (p1 == string::npos) continue;
            size_t p2 = line.find('|', p1 + 1);
            if (p2 == string::npos) continue;

            Task t;
            t.highPriority = (line.substr(0, p1) == "1");
            t.done = (line.substr(p1 + 1, p2 - p1 - 1) == "1");
            t.text = line.substr(p2 + 1);
            tasks.push_back(t);
        }
    }

    void sortTasksByPriority() {
        stable_sort(tasks.begin(), tasks.end(), [](const Task& a, const Task& b) {
            return a.highPriority > b.highPriority;
        });
    }

    bool parseIndex(const string& arg, int& idx) {
        if (arg.empty()) return false;
        for (size_t i = 0; i < arg.size(); ++i) {
            if (!isdigit(static_cast<unsigned char>(arg[i]))) return false;
        }
        try {
            idx = stoi(arg);
        } catch (...) {
            return false;
        }
        return idx >= 1 && idx <= static_cast<int>(tasks.size());
    }

public:
    TodoList() {
        loadFromFile();
        sortTasksByPriority();
    }

    string getWelcome() {
        return "Welcome to TodoList Server! Commands: ADD [HIGH|LOW] <text>, LIST, DONE <n>, DEL <n>, COUNT, CLEAR, HELP, QUIT";
    }

    string handleCommand(const string& rawCommand) {
        string command = trimStr(rawCommand);
        if (command.empty()) {
            return "ERROR empty command";
        }

        istringstream in(command);
        string cmd;
        in >> cmd;
        cmd = toUpperStr(cmd);

        string rest;
        getline(in, rest);
        rest = trimStr(rest);

        if (cmd == "HELP") {
            if (!rest.empty()) return "ERROR HELP takes no arguments";
            return "Commands: ADD buy milk | ADD HIGH finish project | LIST | DONE 1 | DEL 1 | COUNT | CLEAR | QUIT";
        }

        if (cmd == "ADD") {
            if (rest.empty()) {
                return "ERROR missing task text";
            }
            istringstream argStream(rest);
            string firstWord;
            argStream >> firstWord;
            string upperFirst = toUpperStr(firstWord);

            bool isHigh = false;
            string taskText = rest;

            if (upperFirst == "HIGH" || upperFirst == "LOW") {
                string afterPriority;
                getline(argStream, afterPriority);
                afterPriority = trimStr(afterPriority);
                if (afterPriority.empty()) {
                    return "ERROR missing task text after priority";
                }
                isHigh = (upperFirst == "HIGH");
                taskText = afterPriority;
            }

            // Заменяем ';' и '|' на безопасные символы, чтобы не ломать протокол и файл
            for (size_t i = 0; i < taskText.size(); ++i) {
                if (taskText[i] == ';') taskText[i] = ',';
                if (taskText[i] == '|') taskText[i] = '/';
            }

            Task newTask;
            newTask.text = taskText;
            newTask.done = false;
            newTask.highPriority = isHigh;

            tasks.push_back(newTask);
            sortTasksByPriority();
            saveToFile();

            int position = 1;
            for (size_t i = 0; i < tasks.size(); ++i) {
                if (tasks[i].text == newTask.text && tasks[i].highPriority == newTask.highPriority && !tasks[i].done) {
                    position = static_cast<int>(i + 1);
                }
            }
            return "OK, task " + to_string(position) + (isHigh ? " [HIGH]" : "");
        }

        if (cmd == "LIST") {
            if (!rest.empty()) return "ERROR LIST takes no arguments";
            if (tasks.empty()) {
                return "List is empty";
            }
            string result;
            for (size_t i = 0; i < tasks.size(); ++i) {
                if (i > 0) result += "; ";
                result += to_string(i + 1) + ") ";
                if (tasks[i].done) result += "[x] ";
                if (tasks[i].highPriority) result += "[HIGH] ";
                result += tasks[i].text;
            }
            return result;
        }

        if (cmd == "DONE") {
            if (rest.empty()) return "ERROR missing task number";
            istringstream argStream(rest);
            string numStr, extra;
            argStream >> numStr >> extra;
            if (!extra.empty()) return "ERROR too many arguments";

            int idx = 0;
            if (!parseIndex(numStr, idx)) {
                return "ERROR invalid task number";
            }
            tasks[idx - 1].done = true;
            saveToFile();
            return "OK, task " + to_string(idx) + " marked as done";
        }

        if (cmd == "DEL") {
            if (rest.empty()) return "ERROR missing task number";
            istringstream argStream(rest);
            string numStr, extra;
            argStream >> numStr >> extra;
            if (!extra.empty()) return "ERROR too many arguments";

            int idx = 0;
            if (!parseIndex(numStr, idx)) {
                return "ERROR invalid task number";
            }
            tasks.erase(tasks.begin() + (idx - 1));
            saveToFile();
            return "OK, task " + to_string(idx) + " deleted";
        }

        if (cmd == "COUNT") {
            if (!rest.empty()) return "ERROR COUNT takes no arguments";
            int total = static_cast<int>(tasks.size());
            int doneCount = 0;
            for (size_t i = 0; i < tasks.size(); ++i) {
                if (tasks[i].done) doneCount++;
            }
            return "total " + to_string(total) + ", done " + to_string(doneCount);
        }

        if (cmd == "CLEAR") {
            if (!rest.empty()) return "ERROR CLEAR takes no arguments";
            tasks.clear();
            saveToFile();
            return "OK, list cleared";
        }

        return "ERROR unknown command (type HELP)";
    }
};

TodoList todoApp;

string onConnect() {
    return todoApp.getWelcome();
}

string handle(const string& command) {
    return todoApp.handleCommand(command);
}

// ============================================================
// СЕТЕВАЯ ЧАСТЬ (ШАБЛОН)
// ============================================================

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

    SocketType serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock == INVALID_SOCK) {
        cerr << "Socket creation failed\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        cerr << "Bind failed on port " << PORT << "\n";
        CLOSE_SOCKET(serverSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(serverSock, 5) < 0) {
        cerr << "Listen failed\n";
        CLOSE_SOCKET(serverSock);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    cout << "Server started on port " << PORT << ". Waiting for clients...\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        SocketType clientSock = accept(serverSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientSock == INVALID_SOCK) {
            cerr << "Accept failed\n";
            continue;
        }

        cout << "Client connected.\n";
        sendLine(clientSock, onConnect());

        string command;
        while (recvLine(clientSock, command)) {
            string trimmed = trimStr(command);
            if (toUpperStr(trimmed) == "QUIT") {
                sendLine(clientSock, "BYE");
                break;
            }
            string response = handle(trimmed);
            if (!sendLine(clientSock, response)) {
                break;
            }
        }

        cout << "Client disconnected.\n";
        CLOSE_SOCKET(clientSock);
    }

    CLOSE_SOCKET(serverSock);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
