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
    string  Date        = "";
    string  FullName   = "";
};
inline void to_json(json& j, const SinhVien& sv) {
    j = json{{"maSv",        sv.MaSv},
             {"className",   sv.ClassName},
             {"ten",           sv.FullName} };
}


#endif //LTHTM_SINHVIEN_H