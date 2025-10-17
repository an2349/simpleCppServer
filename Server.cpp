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

void checkRequest(int &fd, string mac, const int &epfd) {
    char buf[MAX_HEADER_LENGTH];
    vector<char> *req = new vector<char>();
    vector<char> head;
    int contentLength = 0;
    string header;
    bool headerParsed = false;
    while (!headerParsed) {
        ssize_t n = read(fd, buf, 256);
        if (n <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                this_thread::sleep_for(chrono::milliseconds(5));
                continue;
            } else {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                return;
            }
        }
        req->insert(req->end(), buf, buf + n);
        header += buf;
        size_t endHead= header.find("\r\n\r\n");
        if (endHead == string::npos) {
            string res = Response<string>().build(400, "Request khong hop le", new string());
            send(fd, res.c_str(), res.size(), 0);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            return;
        }
        head.insert(head.end(), buf, buf + endHead + 4);
        headerParsed = true;
    }
    size_t clPos = header.find("Content-Length:");
    if (clPos != string::npos) {
        clPos += strlen("Content-Length:");
        while (clPos < head.size() && header[clPos] == ' ') clPos++;
        size_t endLine = header.find("\r\n", clPos);
        if (endLine == string::npos) endLine = header.size();
        string clStr = header.substr(clPos, endLine - clPos);
        try {
            contentLength = stoul(clStr);
        } catch (...) {
            string res = Response<string>().build(400, "Request khong hop le", new string());
            send(fd, res.c_str(), res.size(), 0);
            epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
            close(fd);
            return;
        }
    }
    if (contentLength > MAX_CONTENT_LENGTH || contentLength < 0) {
        string res = Response<string>().build(400, "Kich thuoc khong hop le", new string());
        send(fd, res.c_str(), res.size(), 0);
        epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
        close(fd);
        return;
    }
    int readData = (int) (contentLength + (int)head.size());
    while (req->size() < readData) {
        ssize_t n = read(fd, buf, min(readData, (int) sizeof(buf)));
        //cout<<n<<endl;
        if (n <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                this_thread::sleep_for(chrono::milliseconds(5));
                continue;
            } else {
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                delete req;
                return;
            }
        }
        req->insert(req->end(), buf, buf + n);
    }
    string response;
    try {
        response = controller->handleRequestAsync(req, mac);
    } catch (...) {
        response = Response<string>().build(400, "ok", new string());
        cout << "loi o ctl\n";
    }
    delete req;
    send(fd, response.c_str(), response.size(), 0);
    close(fd);
    return;
}

void stopServer() {
    if (serverFd != -1) {
        deletePointer();
        shutdown(serverFd, SHUT_RDWR);
        close(serverFd);
        serverFd = -1;
    }
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
    ThreadPool threadPool(MAX_THREADS);
    while (true) {
        int nfds = epoll_wait(epfd, event, MAX_EVENT, -1);
        for (int i = 0; i < nfds; i++) {
            //string mac = "";
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
                //mac = getMAC(clientIP);
                // cout << clientIP << " vua thuc hien ket noi.\nMAC : " << mac << endl;
                fcntl(clientFd,F_SETFL, O_NONBLOCK);
                struct epoll_event clienEvent;
                clienEvent.events = EPOLLIN;
                clienEvent.data.fd = clientFd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, clientFd, &clienEvent);
            } else {
                threadPool.enqueue(checkRequest, clientFd, "", epfd);
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
