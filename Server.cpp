#include <iostream>
#include <future>
#include "configs.h"
#include "Controller/Controller.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "Services/CacheService.h"

using namespace std;

future<void> ServerThread;
int serverStatus = -1;

void startServer(const string &className) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int turn = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        //cerr << "Socket failed\n";
        return;
    }
    serverStatus = server_fd;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &turn, sizeof(turn))) {
        //cerr << "setsockopt failed\n";
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        //cerr << "Bind failed\n";
        return;
    }

    if (listen(server_fd, 3) < 0) {
        //cerr << "Listen failed\n";
        return;
    }
    //scout << "Server đang lắng nghe trên port " << PORT << "\n";
    //DBPool dbPool;//bee boi
    Controller controller;
    CacheService cacheService;
    cacheService.loadCache(className);
    vector<future<void> > futures;
    while (true) {
        if (serverStatus == -1) { break; }
        if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
            //cerr << "Accept failed\n";
            continue;
        }

        futures.push_back(async(launch::async, [new_socket, &controller]() {
            vector<char> *data = new vector<char>();
            char buffer[8192];
            ssize_t n;
            bool isError = false;

            int flags = fcntl(new_socket, F_GETFL, 0);
            fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);

            size_t totalRead = 0;
            size_t contentLength = 0;
            bool headerParsed = false;

            while (true) {
                n = read(new_socket, buffer, sizeof(buffer));
                if (n > 0) {
                    data->insert(data->end(), buffer, buffer + n);
                    totalRead += n;

                    if (!headerParsed) {
                        string requestStr(data->begin(), data->end());
                        size_t pos = requestStr.find("\r\n\r\n");
                        if (pos != string::npos) {
                            headerParsed = true;
                            size_t clPos = requestStr.find("Content-Length:");
                            if (clPos != string::npos) {
                                clPos += 15;
                                size_t endLine = requestStr.find("\r\n", clPos);
                                string clStr = requestStr.substr(clPos, endLine - clPos);
                                contentLength = stoi(clStr);
                                if (contentLength >= MAX_CONTENT_LENGTH) {
                                    Response<string> response;
                                    string res = response.build(400, "kich thuoc khong hop le", new string());
                                    send(new_socket, res.c_str(),
                                         res.size(), 0);
                                    close(new_socket);
                                    isError = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (headerParsed && totalRead >= contentLength) break;
                    if (data->size() > contentLength) {
                        Response<string> response;
                        string res = response.build(413, "Request qua lon", new string());
                        send(new_socket, res.c_str(), res.size(), 0);
                        close(new_socket);
                        isError = true;
                        break;
                    }
                } else if (n == 0) {
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        this_thread::sleep_for(chrono::milliseconds(10));
                        continue;
                    } else {
                        break;
                    }
                }
            }
            if (!isError) {
                auto response_future = controller.handleRequestAsync(data);
                string response = response_future.get();
                delete data;

                send(new_socket, response.c_str(), response.size(), 0);
                close(new_socket);
            }
            else {
                string response = Response<string>().build(400, "Alo ALo", new string());
                send(new_socket, response.c_str(), response.size(), 0);
                                close(new_socket);

            }
        }));
    }
}

void stopServer() {
    if (serverStatus != -1) {
        shutdown(serverStatus, SHUT_RDWR);
        close(serverStatus);
        serverStatus = -1;
    }
}

void restartServer(const string &className) {
    stopServer();
    while (serverStatus != -1) { sleep(0.5); }
    ServerThread = async(launch::async, startServer, className);
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
