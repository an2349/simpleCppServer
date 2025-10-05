//
// Created by an on 10/5/25.
//

#ifndef LTHTM_QUERRY_H
#define LTHTM_QUERRY_H
#include <string>
#include <ctime>
using namespace std;

class Querry {
public:
    enum class querryType {
        KiemTraSinhVien,
        InsertDiemDanh,
    };
    string GetQuerry(const querryType& querryType,const string& maSv,const string& className="");

};


#endif //LTHTM_QUERRY_H