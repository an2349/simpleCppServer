//
// Created by an on 10/4/25.
//

#ifndef LTHTM_CHECKINSERVICE_H
#define LTHTM_CHECKINSERVICE_H
#include "../Model/Response.h"
#include "../Model/SinhVien.h"
#include <future>
#include <string>
#include "../Repo/PoolManager.h"
#include "../Repo/SinhVienRepo.h"
#include "../Repo/Querry.h"
using namespace std;

class CheckInService {
private:
    DBPool& dbPool;
    SinhVienRepo repo;
    Querry querry;
    future<SinhVien> CheckSinhVien(shared_ptr<DBConnection>& conn,const string& maSv);
    future<bool> CheckInSinhVien(shared_ptr<DBConnection>& conn,const string& maSv,string& className);
public:
    CheckInService(): dbPool(g_DBPool) {}
    //CheckInService(DBPool& pool):dbPool(pool) {}
    future<string> CheckInAsync(const string& maSv);
};


#endif //LTHTM_CHECKINSERVICE_H