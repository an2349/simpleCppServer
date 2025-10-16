//
// Created by an on 10/4/25.
//

#ifndef LTHTM_CHECKINSERVICE_H
#define LTHTM_CHECKINSERVICE_H
#include "../Model/Response.h"
#include "../Model/SinhVien.h"
#include <future>
#include <string>
#include "CacheService.h"
#include "../Repo/SinhVienRepo.h"
#include "../Repo/Querry.h"
#include "../Model/Cache.h"
#include "../Repo/DbPool.h"
class CacheService;
class DBPool;
using namespace std;

class CheckInService {
private:
    DBPool& dbPool;
    SinhVienRepo repo;
    CacheService* cacheService;// = nullptr;
    Querry querry;
    SinhVien CheckSinhVien(shared_ptr<DBConnection>& conn,const string& maSv);
    bool CheckInSinhVien(shared_ptr<DBConnection>& conn,const string& maSv,string& macAdress);
public:
    void setCacheService(CacheService& cache) {
        this->cacheService = &cache;
    }
    CheckInService(DBPool& dbPools) : dbPool(dbPools) {
    }

    vector<struct DiemDanh> GetAllSinhVien(const string& className);
    future<string> CheckInAsync(const string& maSv, const string& macAdress);
};


#endif //LTHTM_CHECKINSERVICE_H