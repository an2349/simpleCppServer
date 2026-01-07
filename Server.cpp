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
#include <sys/eventfd.h>
#include <cstring>

using namespace std;

thread ServerThread;
int serverFd = -1;
int epfd = -1;
int notifyFd = -1;
DBPool *dbPool = nullptr;
Controller *controller = nullptr;
CheckInService *checkInService = nullptr;
CacheService *cacheService = nullptr;
FileUpLoadServices *fileUpLoadService = nullptr;
ThreadPool *threadPool = nullptr;

struct ClientInfo {
    string ClientMac;
    string ClientIp;
    time_t lastActive;
    size_t headerLen;
    size_t contenLen;
    int requestCount = 0;
    vector<char>* byteRead;
    bool isHead = false;
    bool keepAlive = false;
};

unordered_map<int, ClientInfo *> clientMap;
mutex clientMapMutex;


struct HttpRequest {
    vector<char> *req = nullptr;
    bool keepAlive;
    bool isContinue = false;
};


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


struct HttpRequest *checkRequest(int fd, const int &epfd) {
    char *buf = new char[MAX_HEADER_LENGTH];
    vector<char> req;
    string *header = new string();
    size_t totalRead = 0;
    bool keepAlive = false;
    ssize_t n = read(fd, buf, MAX_HEADER_LENGTH);
    clientMapMutex.lock();
    if (clientMap.count(fd)) clientMap[fd]->lastActive = time(nullptr);
    clientMapMutex.unlock();
    if (n > 0) {
        if (clientMap[fd]->byteRead == nullptr) {
            clientMapMutex.lock();
            clientMap[fd]->byteRead = new vector<char>();
            clientMapMutex.unlock();
        }
        req.insert(req.end(),clientMap[fd]->byteRead->begin(), clientMap[fd]->byteRead->end());
        req.insert(req.end(), buf, buf + n);
        clientMapMutex.lock();
        clientMap[fd]->byteRead->insert(clientMap[fd]->byteRead->end(), buf, buf + n);
        clientMapMutex.unlock();
        /*cout<<buf<<endl;*/
        size_t totalRead = clientMap[fd]->byteRead->size();
        if (!clientMap[fd]->isHead) {
            header->append(buf, n);
            if (totalRead > MAX_CONTENT_LENGTH) {
                string response = Response<string>().build(400, "Vuot qua do dai", new string());
                send(fd, response.c_str(), response.size(), 0);
                delete header;
                delete[] buf;
                return nullptr;
            }
            size_t headerEnd = header->find("\r\n\r\n");
            if (headerEnd != string::npos) {
                clientMapMutex.lock();
                clientMap[fd]->isHead = true;
                clientMap[fd]->headerLen= headerEnd;
                clientMapMutex.unlock();
                if (header->find("Transfer-Encoding: chunked") != string::npos) {
                    string response = Response<string>().build(400, "Chunked not supported", new string());
                    send(fd, response.c_str(), response.size(), 0);
                    delete header;
                    delete[] buf;
                    return nullptr ;
                }
                if (header->find("Connection: keep-alive") != string::npos ||
                        header->find("Connection: close") == string::npos) {
                    clientMapMutex.lock();
                    clientMap[fd]->keepAlive = true;
                    clientMapMutex.unlock();
                        }
                size_t clPos = header->find("Content-Length:");
                size_t contentLength = 0;
                if (clPos != string::npos) {
                    clPos += strlen("Content-Length:");
                    while (clPos < header->size() && (*header)[clPos] == ' ') clPos++;
                    size_t endLine = header->find("\r\n", clPos);
                    if (endLine == string::npos) endLine = header->size();
                    try {
                        contentLength = std::stoul(header->substr(clPos, endLine - clPos));
                    } catch (...) {
                        contentLength = 0;
                    }
                    clientMapMutex.lock();
                    clientMap[fd]->contenLen = contentLength;
                    clientMapMutex.unlock();
                    if (req.size() < headerEnd + 4 + contentLength) {
                        delete header;
                        delete[] buf;
                        HttpRequest* returnData = new HttpRequest();
                        returnData->isContinue = true;
                        return returnData;
                    }
                }
            }
        }
    }
            else if (n == 0) {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                delete header;
                delete[] buf;
                return nullptr;
            } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
                //this_thread::sleep_for(chrono::milliseconds(10));
                delete header;
                delete[] buf;
                HttpRequest* returnData = new HttpRequest();
                returnData->isContinue = true;
                return returnData;

            } else {
                delete[] buf;
                delete header;
                return nullptr;
            }
    if (req.size() <  clientMap[fd]->headerLen + 4 + clientMap[fd]->contenLen) {
        delete header;
        delete[] buf;
        HttpRequest* returnData = new HttpRequest();
        returnData->isContinue = true;
        return returnData;
    }
    HttpRequest* returnData = new HttpRequest();
    returnData->req = new vector<char>(req.begin(), req.end());
    returnData->keepAlive = clientMap[fd]->keepAlive;
    delete[] buf;
    delete header;
    return returnData;
        }

void stopServer() {
    if (serverFd != -1) {
        if (notifyFd != -1) {
            uint64_t u = 1;
            write(notifyFd, &u, sizeof(u));
        }

        deletePointer();
        shutdown(serverFd, SHUT_RDWR);
        if (threadPool != nullptr) {
            delete threadPool;
            threadPool = nullptr;
        }
        close(serverFd);
        serverFd = -1;
    }

    if (epfd != -1) {
        close(epfd);
        epfd = -1;
    }
    if (notifyFd != -1) {
        close(notifyFd);
        notifyFd = -1;
    }
}


void handle(HttpRequest *request, ClientInfo *clientInfo, const int &fd, const int &epfd) {
    string response = controller->handleRequestAsync(request->req, clientInfo->ClientMac, clientInfo->ClientIp);
    send(fd, response.c_str(), response.size(), 0);
 //kiem co keep-alive khong
    if (!request->keepAlive) {
        {
            lock_guard<mutex> lg(clientMapMutex);
            auto it = clientMap.find(fd);
            if (it != clientMap.end()) {
                delete it->second->byteRead;
                delete it->second;
                clientMap.erase(it);
            }
        }
        close(fd);//dong ketn
    } else {
        clientInfo->requestCount++;
        struct epoll_event ev{};
        ev.events = EPOLLIN;
        ev.data.fd = fd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    }

    delete request->req;
    delete request;
}

void startServer(const vector<string> &className) {
    struct sockaddr_in address;
    /*int turn = 1; // bien bat tat server*/
    int addrlen = sizeof(address);

    if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        cerr << " loi o socket\n";
        return;
    }
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        cerr << "setsockopt SO_REUSEADDR failed\n";
    }
#ifdef SO_REUSEPORT
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        cerr << "setsockopt SO_REUSEPORT failed\n";
    }
#endif

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
    epfd = epoll_create1(0);
    if (epfd == -1) {
        cerr << "epoll_create1 failed\n";
        return;
    }

    notifyFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (notifyFd == -1) {
        cerr << "eventfd failed\n";
        close(epfd);
        epfd = -1;
        return;
    }

    struct epoll_event ev{};
    struct epoll_event event[MAX_EVENT];

    ev.events = EPOLLIN;
    ev.data.fd = notifyFd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, notifyFd, &ev);

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
        //cout << "\r\nDanh sach sinh vien trong lop " << className << " : \n";
        for (const auto &[maSv, dd]: Cache::danhSachSV) {
            cout << "Ma so: " << maSv
                    << ", Ten: " << dd.FullName
                    << ", Lop: " << dd.ClassName
                    << ", MAC: " << dd.Mac
                    << ", Diem danh: " << dd.IsCheckIn
                    << endl;
        }
        checkInService->setCacheService(*cacheService);
    } catch (...) {
        stopServer();
        return;
    }
    threadPool = new ThreadPool(MAX_THREADS);
    while (serverFd != -1) {
        int nfds = epoll_wait(epfd, event, MAX_EVENT, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            break;
        }
        static time_t lastCheck = 0;
        time_t now = time(nullptr);
        if (now - lastCheck >= 5) {//client ma giu qua 5s server se ngat ket noi
            lastCheck = now;
            lock_guard<mutex> lock(clientMapMutex);
            for (auto it = clientMap.begin(); it != clientMap.end();) {
                if (now - it->second->lastActive > 5) {
                    close(it->first);
                    delete it -> second->byteRead;
                    delete it->second;
                    it = clientMap.erase(it);
                } else {
                    ++it;
                }
            }
        }

        for (int i = 0; i < nfds; i++) {
            int clientFd = event[i].data.fd;
            if (clientFd == notifyFd) {
                uint64_t val;
                read(notifyFd, &val, sizeof(val));
                serverFd = -1;
                break;
            }
            if (clientFd == serverFd) {
                sockaddr_in clientAddr{};
                socklen_t clienLen = sizeof(clientAddr);
                int newFd = accept(serverFd, (struct sockaddr *) &clientAddr, (socklen_t *) &clienLen);
                if (newFd == -1) {
                    continue;
                }
                char clientIP[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
                string mac = getMAC(clientIP);
                auto *info = new ClientInfo();
                info->ClientMac = mac;
                info->ClientIp = clientIP;
                info->lastActive = time(nullptr);
                lock_guard<mutex> lock(clientMapMutex);
                clientMap[newFd] = info;
                cout << clientIP << " vua thuc hien ket noi.\nMAC : " << mac << endl;
                fcntl(newFd,F_SETFL, O_NONBLOCK);
                struct epoll_event clienEvent;
                clienEvent.events = EPOLLIN;
                clienEvent.data.fd = newFd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, newFd, &clienEvent);
            } else {
                try {
                    auto x = checkRequest(clientFd, epfd);
                if (x == nullptr) {
                    string response = Response<string>().build(500, " khong hop le", new string());
                    send(clientFd, response.c_str(), response.size(), 0);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL);
                    close(clientFd);
                    delete x;
                    break;
                }else if (x->isContinue) {
                    delete x->req;
                    delete x;
                    break;
                } else {
                    ClientInfo *ci = nullptr;
                    {
                        lock_guard<mutex> lg(clientMapMutex);
                        auto it = clientMap.find(clientFd);
                        if (it != clientMap.end()) ci = it->second;
                    }
                    if (ci == nullptr) {
                        string response = Response<string>().build(500, " khong hop le", new string());
                        send(clientFd, response.c_str(), response.size(), 0);
                        epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL);
                        close(clientFd);
                        delete x;
                        continue;
                    }
                    threadPool->enqueue(handle, x, ci, clientFd, epfd);
                }
                }
                catch (exception &e) {
                    cout<<e.what()<<endl;
                    return;
                }
            }
        }
        if (serverFd == -1) break;
    }

    {
        lock_guard<mutex> lock(clientMapMutex);
        for (auto &p: clientMap) {
            close(p.first);
            delete p.second->byteRead;
            delete p.second;
        }
        clientMap.clear();
    }
    if (epfd != -1) {
        close(epfd);
        epfd = -1;
    }
    if (notifyFd != -1) {
        close(notifyFd);
        notifyFd = -1;
    }
}


void restartServer(const vector<string> &className) {
    stopServer();
    while (serverFd != -1) { sleep(0.5); }
    startServer(className);
}

int handleCommand() {
    /*while (true) {*/
        string input;
        getline(cin, input);

        istringstream iss(input);
        string command;
        iss >> command;

        if (command == "start") {
            string option;
            vector<string> classNames;
            iss >> option;

            if (option == "-c") {
                string list;
                getline(iss, list);
                stringstream ss(list);
                string cls;
                while (getline(ss, cls, ',')) {
                    cls.erase(remove_if(cls.begin(), cls.end(), ::isspace), cls.end());
                    if (!cls.empty()) classNames.push_back(cls);
                }
            }

            if (serverFd == -1) {
                cout << "Dang khoi dong server\n";
                ServerThread = thread(startServer, classNames); //(launch::async, startServer, classNames);
                sleep(3);
            } else {
                cout << "Server dang chay roi\n";
            }
        } else if (command == "stop") {
            if (serverFd != -1) {
                cout << "Dung server\n";
                stopServer();
                if (ServerThread.joinable()) ServerThread.join();
            } else {
                cout << "Server dang khong hoat dong\n";
            }
        } else if (command == "restart") {
            string option;
            vector<string> classNames;
            iss >> option;

            if (option == "-c") {
                string list;
                getline(iss, list);
                stringstream ss(list);
                string cls;
                while (getline(ss, cls, ',')) {
                    cls.erase(remove_if(cls.begin(), cls.end(), ::isspace), cls.end());
                    if (!cls.empty())
                        classNames.push_back(cls);
                }
            } else {
                if (!option.empty())
                    classNames.push_back(option);
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
        } else if (command == "check") {
            if (serverFd == -1) {
                cout << "Server chưa chạy\n";
                //continue;
            }
            for (const auto &[maSv, dd]: Cache::danhSachSV) {
                cout << "Ma so: " << maSv
                        << ", Ten: " << dd.FullName
                        << ", Lop: " << dd.ClassName
                        << ", MAC: " << dd.Mac
                        << ", Diem danh: " << dd.IsCheckIn
                        << endl;
            }
        } else if (command == "thoat") {
            stopServer();
            return -1;
        } else if (command == "help") {
            cout << "Danh sách lệnh:\n";
            cout << "   start                                      : Khởi động máy chủ\n";
            cout << "  stop                                       : Dừng máy chủ\n";
            cout << "  restart                                    : Khởi động lại máy chủ\n";
            cout << " check                                      : Xem thông tin điểm danh" << endl;
            cout << "  thoat                                      : Thoát khỏi máy chủ\n";
            cout << "  help                                       : Thông tin lệnh\n";
            cout << " -c   <lớp1,lớp2,...>                  : danh sách lớp { start -c ..., restart -c ... }\n";
        } else {
            cout << "Khong hop le\n";
        }
    return 0;
}
int main() {
    try {
        int status = 0;
        while (status != -1) {
            cout << "\rServer :";
            status = handleCommand();
        }
    }
    catch (const exception &e) {
        cerr << e.what() << endl;
    }
    catch (...) {
        cerr << "Unknown exception!" << endl;
    }
    cout<<"Bye"<<endl;
    return 0;
}