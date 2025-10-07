#include <iostream>
#include <future>
#include "configs.h"
#include "Controller/Controller.h"
#include <arpa/inet.h>
#include <unistd.h>
#include "Services/CacheService.h"

using namespace std;

future<void> ServerThread;
int serverStatus = -1;
void startServer(const string& className) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int turn = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        //cerr << "Socket failed\n";
        return;
    }
    serverStatus =server_fd;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &turn, sizeof(turn))) {
        //cerr << "setsockopt failed\n";
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
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
    vector<future<void>> futures;
    while (true) {
        if (serverStatus == -1 ){break;}
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            //cerr << "Accept failed\n";
            continue;
        }

        futures.push_back(async(launch::async, [new_socket, &controller]() {
            char buffer[1024] = {0};
            read(new_socket, buffer, 1024);
            string request(buffer);
            auto response_future = controller.handleRequestAsync(request);
            string response = response_future.get();

            send(new_socket, response.c_str(), response.size(), 0);
            close(new_socket);
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

void restartServer(const string& className) {
    stopServer();
    while (serverStatus != -1){ sleep(0.5);}
    ServerThread = async(launch::async, startServer,className);
}

int main() {
    while(true){
        string input;
        cout<<"\rServer :";
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
        }
        else if (command == "stop") {
            if (serverStatus != -1) {
                cout<<"Dung server\n";
                stopServer();
            } else {
                cout<<"Server dang khong hoat dong\n";
            }
        }
        else if (command == "restart") {
            string option;
            string classNames;

            iss >> option;
            if (option == "-c") {
                iss >> classNames;
            } else {
                classNames = "";
            }

            cout<<"Dang khoi dong lai server"<<flush;
            sleep(1.7);cout<<"."<<flush; sleep(1); cout<<"."<<flush; sleep(1); cout<<"."<<flush; sleep(2);
            restartServer(classNames);
            cout<<"\rKhoi dong lai thanh cong!\n";
        }

        else if (command == "thoat") {
            stopServer();
            cout<<"Bye\n";
            return 0;
        }
        else {
            cout<<"Khong hop le\n";
        }
    }
    return 0;
}
