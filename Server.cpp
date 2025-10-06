#include <iostream>
#include <future>
#include "configs.h"
#include "Controller/Controller.h"
#include <arpa/inet.h>
#include <unistd.h>
using namespace std;

future<void> ServerThread;
int serverStatus = -1;
void startServer() {
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
    DBPool dbPool;//bee boi

    Controller controller;
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

void restartServer() {
    stopServer();
    sleep(2);
    ServerThread = async(launch::async, startServer);
}

int main() {
    while(true){
        string input;
        cout<<"server :";
        cin>>input;
        if ( input== "start") {
            if (serverStatus == -1) {
                cout<<"Khoi dong server\n";
                ServerThread = async(launch::async, startServer);
            }
            else {
                cout<<"Server dang chay roi\n";
            }
        }
        if (input == "stop") {
            if (serverStatus != -1) {
                cout<<"Dung server\n";
                stopServer();
            }
            else{
                cout<<"Server dang khong hoat dong\n";
            }
        }
        if (input == "restart"){
            if (serverStatus == -1) {
                cout<<"restart server\n";
                restartServer();
            }
            else {
                ServerThread = async(launch::async, startServer);
            }
        }
    }
    return 0;
}
