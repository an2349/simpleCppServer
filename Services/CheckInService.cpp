#include "CheckInService.h"
future<string> CheckInService::CheckInAsync(const string& maSv) {
    return async(launch::async, [maSv, this]() {
        Response<SinhVien> response;
        SinhVien* sinhVien = new SinhVien();

        auto conn = dbPool.getConn();
        if (!conn || !conn->get()) {
            return response.build(400,"DB erorr",sinhVien);
        }

        try {
            auto f = CheckSinhVien(conn,maSv);
            SinhVien sv= f.get();
            if(sv.MaSv== ""||sv.ClassName == "") {
                dbPool.closeConn(conn);
                return response.build(400,"Sinh vien khong ton tai trong lop",sinhVien);
            }
            else {
                time_t t = time(nullptr);
                tm* now = localtime(&t);
                char buffer[11];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
                string today(buffer);
                if(today == sv.Date) {
                    return response.build(400,"Sinh vien da diem danh roi",sinhVien);
                }
            }
            auto i = CheckInSinhVien(conn,maSv,sv.ClassName);
            bool success = i.get();
            if (!success) {
                dbPool.closeConn(conn);
                return response.build(400,"loi xu li,vui long thu lai",sinhVien);
            }

            dbPool.closeConn(conn);
            sinhVien->MaSv = maSv;
            sinhVien->ClassName = sv.ClassName;
            sinhVien->FullName = sv.FullName;
            return response.build(200,"Sinh Vien "+ maSv +" diem danh thanh cong",sinhVien);
        }
        catch (...) {
            dbPool.closeConn(conn);
            return response.build(500,"UN_DETECTED_EROR",sinhVien);
        }
    });
}
 future<SinhVien> CheckInService::CheckSinhVien(shared_ptr<DBConnection>& conn,const string& maSv) {
    return async(launch::async, [maSv, this, conn]() {
        try {
             return repo.KiemTra(conn,querry.GetQuerry(Querry::querryType::KiemTraSinhVien,maSv));
        }
        catch (...) {
            SinhVien sivi;
            return sivi;
        }
    });
}
future<bool> CheckInService::CheckInSinhVien(shared_ptr<DBConnection>& conn,const string& maSv,string& className) {
    return async(launch::async, [maSv, this, className, conn]() {
        return repo.DiemDanh(conn,querry.GetQuerry(Querry::querryType::InsertDiemDanh,maSv,className));
    });
}