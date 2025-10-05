//
// Created by an on 10/5/25.
//

#ifndef LTHTM_SINHVIENREPO_H
#define LTHTM_SINHVIENREPO_H
#include <memory>
#include <string>
#include "../Model/SinhVien.h"

class DBConnection;
using  namespace  std;

class SinhVienRepo {
public:
    SinhVienRepo(){}
    bool DiemDanh(const shared_ptr<DBConnection>& conn,const string& querry);
    SinhVien KiemTra(const shared_ptr<DBConnection>& conn,const string& querry);
};


#endif //LTHTM_SINHVIENREPO_H