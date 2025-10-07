#include "CheckInService.h"
future<string> CheckInService::CheckInAsync(const string& maSv,const string& macAdress) {
    return async(launch::async, [maSv, macAdress, this]() {
        Response<SinhVien> response;
        SinhVien* sinhVien = new SinhVien();

        auto conn = dbPool.getConn();
        if (!conn || !conn->get()) {
            return response.build(400,"DB erorr",sinhVien);
        }

        try {
            auto f = CheckSinhVien(conn,maSv);
            *sinhVien = f.get();
            if(sinhVien->MaSv== ""||sinhVien->ClassName == "") {
                dbPool.closeConn(conn);
                return response.build(400,"Sinh vien khong ton tai trong lop",sinhVien);
            }
            else {
                time_t t = time(nullptr);
                tm* now = localtime(&t);
                char buffer[11];
                strftime(buffer, sizeof(buffer), "%Y-%m-%d", now);
                string today(buffer);
                if(today == sinhVien->Date) {
                    return response.build(400,"Sinh vien da diem danh roi",sinhVien);
                }
                else if (sinhVien->Mac != "" && sinhVien->Mac != macAdress) {
                    return response.build(400,"Khong phai thiet bi goc cua sinh vien",sinhVien);
                }
                else {
                    sinhVien->Mac = macAdress;
                }
            }
            auto i = CheckInSinhVien(conn,maSv,sinhVien->Mac);
            bool success = i.get();
            if (!success) {
                dbPool.closeConn(conn);
                return response.build(400,"loi xu li,vui long thu lai",sinhVien);
            }

            dbPool.closeConn(conn);
            return response.build(200,"Sinh Vien "+ sinhVien->FullName +" diem danh thanh cong",sinhVien);
        }
        catch (...) {
            dbPool.closeConn(conn);
            return response.build(500,"UN_DETECTED_EROR",sinhVien);
        }
    });
}
 future<SinhVien> CheckInService::CheckSinhVien(shared_ptr<DBConnection>& conn,const string& maSv) {
    return async(launch::async, [maSv, this, conn](){
        return repo.KiemTra(conn,querry.GetQuerry(Querry::querryType::KiemTraSinhVien,maSv));
    });
}
future<bool> CheckInService::CheckInSinhVien(shared_ptr<DBConnection>& conn,const string& maSv,string& macAdress) {
    return async(launch::async, [maSv, this, macAdress, conn]() {
        return repo.DiemDanh(conn,querry.GetQuerry(Querry::querryType::InsertDiemDanh,maSv,macAdress));
    });
}