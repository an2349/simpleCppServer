//
// Created by an on 10/7/25.
//

#ifndef LTHTM_CACHESERVICE_H
#define LTHTM_CACHESERVICE_H
#include <string>
#include "../Model/Cache.h"
#include "CheckInService.h"

using namespace std;
class CheckInService;


class CacheService {
    public:
    Cache& cache;
    CacheService(CheckInService& checkInServices) :cache(Cache::getInstance()), checkInService(checkInServices) {}
    void loadCache(const string& className);
    struct DiemDanh checkSinhVien (const string& maSv, const string& macAdress);
    struct DiemDanh checkSinhVien (const string& macAdress);
    void updateSinhVien(const string& maSv, const string& macAdress);
private:
    CheckInService& checkInService;
};


#endif //LTHTM_CACHESERVICE_H