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
#include <sys/epoll.h>
#include "ThreadPool.h"

using namespace std;

future<void> ServerThread;
int serverFd = -1;
DBPool *dbPool = nullptr;
Controller *controller = nullptr;
CheckInService *checkInService = nullptr;
CacheService *cacheService = nullptr;
FileUpLoadServices *fileUpLoadService = nullptr;
ThreadPool* threadPool = nullptr;
unordered_map<int, string*> macMap;
mutex macMapMutex;

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


string getMAC(const string &ip) {
    //goi arp de lay mac neu trong mang lan
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

vector<char>* checkRequest(int fd, string *mac, const int &epfd) {
    char* buf = new char[64];
    vector<char>* req = new vector<char>();
    string* header = new string();
    size_t totalRead = 0;

    while (true) {
        ssize_t n = read(fd, buf, 64);
        if (n > 0) {
            req->insert(req->end(), buf, buf + n);
            totalRead += n;
            header->append(buf, n);

            if (totalRead > MAX_CONTENT_LENGTH) {
                string response = Response<string>().build(400, "Vuot qua do dai", new string());
                send(fd, response.c_str(), response.size(), 0);
                break;
            }

            size_t headerEnd = header->find("\r\n\r\n");
            if (headerEnd != string::npos) {
                if (header->find("Transfer-Encoding: chunked") != string::npos) {
                    string response = Response<string>().build(400, "Chunked not supported", new string());
                    send(fd, response.c_str(), response.size(), 0);
                    break;
                }

                size_t clPos = header->find("Content-Length:");
                size_t contentLength = 0;
                if (clPos != string::npos) {
                    clPos += strlen("Content-Length:");
                    while (clPos < header->size() && (*header)[clPos] == ' ') clPos++;
                    size_t endLine = header->find("\r\n", clPos);
                    if (endLine == string::npos) endLine = header->size();
                    try { contentLength = std::stoul(header->substr(clPos, endLine - clPos)); } catch (...) { contentLength = 0; }

                    if (req->size() >= headerEnd + 4 + contentLength) break;
                } else {
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                    //delete req;
                    delete[] buf;
                    delete header;
                    delete macMap[fd];
                    macMap.erase(fd);
                    close(fd);
                    return nullptr;
                }
            }
        }
        else if (n == 0) {
            break;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            this_thread::sleep_for(chrono::milliseconds(10));
            // break;
        }
        else {
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            //delete req;
            delete[] buf;
            delete header;
            delete macMap[fd];
            macMap.erase(fd);
            close(fd);
            return nullptr;
        }
    }
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    //delete req;
    delete[] buf;
    delete header;
    delete macMap[fd];
    macMap.erase(fd);
    close(fd);
    return req;
}
    /*string response;
    try {
        response = controller->handleRequestAsync(req, *mac);
    } catch (...) {
        response = Response<string>().build(400, "loi o ctl", new string());
    }

    send(fd, response.c_str(), response.size(), 0);

    delete req;
    delete[] buf;
    delete header;
    //delete macMap[fd];
    macMap.erase(fd);
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}*/


void stopServer() {
    if (serverFd != -1) {
        deletePointer();
        shutdown(serverFd, SHUT_RDWR);
        if (threadPool != nullptr) {
            delete threadPool;
            threadPool = nullptr;
        }
        close(serverFd);
        serverFd = -1;
    }
}


void handle(vector<char>* req , const string& macID, const int& fd, const int& epfd) {
    string response = controller->handleRequestAsync(req, macID);
    send(fd, response.c_str(), response.size(), 0);
    /*epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);*/
    delete req;
}

void startServer(const string &className) {
    struct sockaddr_in address;
    int turn = 1;
    int addrlen = sizeof(address);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << " loi o socket\n";
        return;
    }
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &turn, sizeof(turn))) {
        cerr << " loi o socket \n";
        return;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(serverFd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        cerr << "loi o bind port\n";
        return;
    }
    //nghe tren cong voi queu la 70
    if (listen(serverFd, MAX_CONN) < 0) {
        cerr << " loi lang nghe\n";
        return;
    }
    int epfd = epoll_create1(0);
    struct epoll_event ev{};
    struct epoll_event event[MAX_EVENT];
    ev.events = EPOLLIN;
    ev.data.fd = serverFd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, serverFd, &ev);
    try {
        dbPool = new DBPool();
        fileUpLoadService = new FileUpLoadServices();
        checkInService = new CheckInService(*dbPool);
        controller = new Controller(*checkInService, *fileUpLoadService);
        cacheService = new CacheService(*checkInService);
        cacheService->loadCache(className);
        cout << "\r\nDanh sach sinh vien trong lop " << className << " : \n";
        for (const auto &[maSv, dd]: Cache::danhSachSV) {
            cout << "Ma so: " << maSv
                    << ", Ten: " << dd.FullName
                    << ", Lop: " << dd.ClassName
                    << ", Diem danh: " << dd.IsCheckIn
                    << ", MAC: " << dd.Mac
                    << endl;
        }
        checkInService->setCacheService(*cacheService);
    } catch (...) {
        stopServer();
        return;
    }
    threadPool  = new ThreadPool(MAX_THREADS);
    while (true) {
        int nfds = epoll_wait(epfd, event, MAX_EVENT, -1);
        for (int i = 0; i < nfds; i++) {
            int clientFd = event[i].data.fd;
            if (clientFd == serverFd) {
                sockaddr_in clientAddr{};
                socklen_t clienLen = sizeof(clientAddr);
                clientFd = accept(serverFd, (struct sockaddr *) &clientAddr, (socklen_t *) &clienLen);
                if (clientFd == -1) {
                    close(clientFd);
                    break;
                }
                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
                string *mac =new string();
                *mac = getMAC(clientIP);
                lock_guard<std::mutex> lock(macMapMutex);
                macMap[clientFd] = mac;
                cout << clientIP << " vua thuc hien ket noi.\nMAC : " << *mac << endl;
                fcntl(clientFd,F_SETFL, O_NONBLOCK);
                struct epoll_event clienEvent;
                clienEvent.events = EPOLLIN;
                clienEvent.data.fd = clientFd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &clienEvent);
            } else {
                if (checkRequest(clientFd, macMap[clientFd], epfd) == nullptr) {
                    string response = Response<string>().build(500, " khong hop le", new string());
                    send(clientFd, response.c_str(), response.size(), 0);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL);
                    close(clientFd);

                }
                threadPool->enqueue(handle, checkRequest(clientFd, macMap[clientFd], epfd), *macMap[clientFd], clientFd , epfd);
            }
        }
    }
}


void restartServer(const string &className) {
    stopServer();
    while (serverFd != -1) { sleep(0.5); }
    startServer(className);
}


int main() {
    startServer("");
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
            if (serverFd == -1) {
                cout << "Dang khoi dong server\n";
                ServerThread = async(launch::async, startServer, classNames);
                sleep(3);
            } else {
                cout << "Server dang chay roi\n";
            }
        } else if (command == "stop") {
            if (serverFd != -1) {
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
