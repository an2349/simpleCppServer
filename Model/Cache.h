//
// Created by an on 10/7/25.
//

#ifndef LTHTM_CACHE_H
#define LTHTM_CACHE_H
#include <string>
#include <unordered_map>
#include <shared_mutex>

using namespace std;
struct DiemDanh {
    bool IsCheckIn = false;
    string MaSv = "";
    string Mac = "";
    string FullName = "";
};
class Cache {
public:
    Cache(){};
    static Cache& getInstance() {
        static Cache instance;
        return instance;
    }
    static unordered_map<string, DiemDanh> danhSachSV;
    static shared_mutex cacheMutex;
};


#endif //LTHTM_CACHE_H