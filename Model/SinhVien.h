//
// Created by an on 10/4/25.
//

#ifndef LTHTM_SINHVIEN_H
#define LTHTM_SINHVIEN_H
#include <string>
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace std;
class SinhVien {
public:
    SinhVien(){}
    SinhVien(string *Lop, string *maSv);
    string  MaSv        = "";
    string  ClassName   = "";
    string  FullName    = "";
    string  Mac         = "";
};
inline void to_json(json& j, const SinhVien& sv) {
    j = json{{"Ma so",      sv.MaSv},
             {"Lop",        sv.ClassName},
             {"Ten",        sv.FullName} };
}


#endif //LTHTM_SINHVIEN_H