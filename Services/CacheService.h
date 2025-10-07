//
// Created by an on 10/7/25.
//

#ifndef LTHTM_CACHESERVICE_H
#define LTHTM_CACHESERVICE_H
#include <string>
#include "../Model/Cache.h"
#include "CheckInService.h"

using namespace std;

class CacheService {
    public:
    Cache& cache;
    CacheService() : cache(Cache::getInstance()) {}
    void loadCache(const string& className);
    future<struct DiemDanh> checkSinhVien (const string& maSv, const string& macAdress);
    void updateSinhVien(const string& maSv, const string& macAdress);
private:
    CheckInService checkInService;
};


#endif //LTHTM_CACHESERVICE_H