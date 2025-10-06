//
// Created by an on 10/5/25.
//

#include "Querry.h"

#include <iostream>
#include <string>

string Querry::GetQuerry(const querryType& querryType,const string& maSv,const string& className) {
    switch (querryType) {
        case querryType::InsertDiemDanh:
            //return "INSERT INTO diem_danh (maSv) VALUES (" + maSv + ");" + "SELECT modified_date FROM diem_danh"
             std::cout<<maSv;
              return "CALL proc_cap_nhat_ngay_dd('"+maSv+"');CALL proc_diem_danh('"+maSv+"');";
        case querryType::KiemTraSinhVien: {
            /*time_t t = time(nullptr);
            tm* now = localtime(&t);
            char buffer[11];
            strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
            string today(buffer);*/
            return "SELECT user_name,class_name,full_name,DATE(modified_date) AS modified_date FROM student WHERE user_name = '" + maSv +"';";//AND DATE(modified_date) = '" + today + "';";
        }
        default:
            return "";
    }
}
