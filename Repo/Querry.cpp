//
// Created by an on 10/5/25.
//

#include "Querry.h"

#include <iostream>
#include <string>

string Querry::GetQuerry(const querryType& querryType,const string& maSv,const string& macAddress) {
    switch (querryType) {
        case querryType::InsertDiemDanh:
              return "CALL proc_diem_danh_tong_hop('"+maSv+"', '"+macAddress+"')";
        case querryType::KiemTraSinhVien: {
            return "SELECT user_name,class_name,full_name,mac,DATE(modified_date) AS modified_date FROM student WHERE user_name = '" + maSv +"'";
        }
        default:
            return "";
    }
}