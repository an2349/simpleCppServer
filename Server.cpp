#include <iostream>
#include <future>
#include "configs.h"
#include "Controller/Controller.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "Services/CacheService.h"
#include "Services/FileUpLoadServices.h"

using namespace std;

future<void> ServerThread;
int serverStatus = -1;
DBPool *dbPool = nullptr;
Controller *controller = nullptr;
CheckInService *checkInService = nullptr;
CacheService *cacheService = nullptr;
FileUpLoadServices *fileUpLoadService = nullptr;

string getMAC(const string &ip) {
    FILE *arpCache = fopen("/proc/net/arp", "r");
    if (!arpCache) return "";
    char line[256];
    fgets(line, sizeof(line), arpCache);
    while (fgets(line, sizeof(line), arpCache)) {
        char ipAddr[32], hwType[32], flags[32], mac[32], mask[32], device[32];
        sscanf(line, "%31s %31s %31s %31s %31s %31s", ipAddr, hwType, flags, mac, mask, device);
        if (ip == ipAddr) {
            fclose(arpCache);
            return string(mac);
        }
    }
    fclose(arpCache);
    return "";
}
void deletePointer() {
    delete controller;
    controller = nullptr;
    delete checkInService;
    checkInService = nullptr;
    delete cacheService;
    cacheService = nullptr;
    delete fileUpLoadService;
    fileUpLoadService = nullptr;
    delete dbPool;
    dbPool = nullptr;
}

void startServer(const string &className) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int turn = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        return;
    }
    serverStatus = server_fd;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &turn, sizeof(turn))) {
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        return;
    }

    if (listen(server_fd, 3) < 0) {
        return;
    }

    dbPool = new DBPool();
    fileUpLoadService = new FileUpLoadServices();
    checkInService = new CheckInService(*dbPool);
    controller = new Controller(*checkInService, *fileUpLoadService);
    cacheService = new CacheService(*checkInService);
    cacheService->loadCache(className);
    checkInService->setCacheService(*cacheService);

    vector<future<void> > futures;

    while (true) {
        if (serverStatus == -1) break;
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
        string mac = getMAC(client_ip);
        cout << client_ip;
        cout << ":" << ntohs(address.sin_port);
        cout << "\nMAC: " << mac << "\nVua thuc hien " ;
        futures.push_back(async(launch::async, [new_socket, &mac]() {
            vector<char> *data = new vector<char>();
            bool isError = false;
            int flags = fcntl(new_socket, F_GETFL, 0);
            fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

            string headerBuf;
            size_t contentLength = 0;
            bool headerParsed = false;
            size_t totalBodyRead = 0;
            const size_t READ_CHUNK = 1024;
            char buffer[READ_CHUNK];

            while (!headerParsed) {
                char ch;
                ssize_t n = read(new_socket, &ch, 1);
                if (n <= 0) {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        this_thread::sleep_for(chrono::milliseconds(5));
                        continue;
                    } else {
                        isError = true;
                        break;
                    }
                }
                data->push_back(ch);
                headerBuf += ch;

                if (headerBuf.size() >= 4 && headerBuf.substr(headerBuf.size() - 4) == "\r\n\r\n") {
                    headerParsed = true;

                    size_t clPos = headerBuf.find("Content-Length:");
                    if (clPos != string::npos) {
                        clPos += strlen("Content-Length:");
                        while (clPos < headerBuf.size() && headerBuf[clPos] == ' ') clPos++;
                        size_t endLine = headerBuf.find("\r\n", clPos);
                        if (endLine == string::npos) endLine = headerBuf.size();
                        string clStr = headerBuf.substr(clPos, endLine - clPos);
                        try {
                            contentLength = stoul(clStr);
                        } catch (...) {
                            string res = Response<string>().build(400, "Content-Length invalid", new string());
                            send(new_socket, res.c_str(), res.size(), 0);
                            close(new_socket);
                            isError = true;
                            break;
                        }

                        if (contentLength > MAX_CONTENT_LENGTH) {
                            string res = Response<string>().build(413, "Request qua lon", new string());
                            send(new_socket, res.c_str(), res.size(), 0);
                            close(new_socket);
                            isError = true;
                            break;
                        }
                    } else {
                        contentLength = 0;
                    }

                    size_t bodyStart = headerBuf.find("\r\n\r\n");
                    size_t extra = headerBuf.size() - (bodyStart + 4);
                    if (extra > 0) {
                        if (extra > contentLength) {
                            string res = Response<string>().build(413, "Request qua lon", new string());
                            send(new_socket, res.c_str(), res.size(), 0);
                            close(new_socket);
                            isError = true;
                            break;
                        }
                        totalBodyRead += extra;
                    }
                    break;
                }
            }

            if (!isError && totalBodyRead < contentLength) {
                while (totalBodyRead < contentLength) {
                    size_t toRead = std::min(READ_CHUNK, contentLength - totalBodyRead);
                    ssize_t n = read(new_socket, buffer, toRead);
                    if (n > 0) {
                        if (totalBodyRead + static_cast<size_t>(n) > MAX_CONTENT_LENGTH) {
                            string res = Response<string>().build(413, "Request qua lon", new string());
                            send(new_socket, res.c_str(), res.size(), 0);
                            close(new_socket);
                            isError = true;
                            break;
                        }
                        data->insert(data->end(), buffer, buffer + n);
                        totalBodyRead += n;
                    } else if (n == 0) {
                        break;
                    } else {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            this_thread::sleep_for(chrono::milliseconds(5));
                            continue;
                        } else {
                            isError = true;
                            break;
                        }
                    }
                }
            }

            if (!isError) {
                auto response_future = controller->handleRequestAsync(data,mac);
                string response = response_future.get();
                delete data;
                send(new_socket, response.c_str(), response.size(), 0);
                close(new_socket);
            } else {
                delete data;
                string response = Response<string>().build(400, "Alo ALo", new string());
                send(new_socket, response.c_str(), response.size(), 0);
                close(new_socket);
            }
        }));
    }
}

void stopServer() {
    if (serverStatus != -1) {
        deletePointer();
        shutdown(serverStatus, SHUT_RDWR);
        close(serverStatus);
        serverStatus = -1;
    }
}

void restartServer(const string &className) {
    stopServer();
    while (serverStatus != -1) { sleep(0.5); }
    startServer(className);
}

int main() {
    while (true) {
        string input;
        cout << "\rServer :";
        getline(cin, input);

        istringstream iss(input);
        string command;
        iss >> command;

        if (command == "start") {
            string option;
            string classNames;

            iss >> option;
            if (option == "-c") {
                iss >> classNames;
            } else {
                classNames = "";
            }
            if (serverStatus == -1) {
                cout << "Khoi dong server\n";
                ServerThread = async(launch::async, startServer, classNames);
            } else {
                cout << "Server dang chay roi\n";
            }
        } else if (command == "stop") {
            if (serverStatus != -1) {
                cout << "Dung server\n";
                stopServer();
            } else {
                cout << "Server dang khong hoat dong\n";
            }
        } else if (command == "restart") {
            string option;
            string classNames;

            iss >> option;
            if (option == "-c") {
                iss >> classNames;
            } else {
                classNames = "";
            }

            cout << "Dang khoi dong lai server" << flush;
            sleep(1.7);
            cout << "." << flush;
            sleep(1);
            cout << "." << flush;
            sleep(1);
            cout << "." << flush;
            sleep(2);
            restartServer(classNames);
            cout << "\rKhoi dong lai thanh cong!\n";
        } else if (command == "thoat") {
            stopServer();
            cout << "Bye\n";
            return 0;
        } else {
            cout << "Khong hop le\n";
        }
    }
    return 0;
}
