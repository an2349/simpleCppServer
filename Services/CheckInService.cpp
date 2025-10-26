#include "CheckInService.h"
#include "CacheService.h"

future<string> CheckInService::CheckInAsync(const string &maSv, const string &macAdress) {
    return async(launch::async, [maSv, macAdress, this]() {
        Response<SinhVien> response;
        SinhVien *sinhVien = new SinhVien();

        auto conn = dbPool.getConn();
        if (!conn || !conn->get()) {
            return response.build(400, "DB erorr", sinhVien);
        }
        try {
            DiemDanh sivi;
            if (maSv.empty()) {
                sivi = cacheService->checkSinhVien(macAdress);
            } else {
                sivi = cacheService->checkSinhVien(maSv, macAdress);
            }
            sinhVien->Mac = macAdress;
            sinhVien->MaSv = sivi.MaSv;
            sinhVien->FullName = sivi.FullName;
            sinhVien->ClassName = sivi.ClassName;
            if (sivi.MaSv.empty()) {
                dbPool.closeConn(conn);
                return response.build(400, "Sinh vien khong ton tai trong lop", sinhVien);
            } else if (sivi.Mac != "" && sivi.Mac != macAdress) {
                dbPool.closeConn(conn);
                return response.build(400, "Khong phai thiet bi dang ky cua sinh vien", sinhVien);
            } else if (sivi.IsCheckIn) {
                dbPool.closeConn(conn);
                return response.build(400, "Sinh vien da diem danh roi", sinhVien);
            }
            bool success = CheckInSinhVien(conn, sinhVien->MaSv, sinhVien->Mac);

            if (!success) {
                dbPool.closeConn(conn);
                return response.build(400, "loi xu li,vui long thu lai", sinhVien);
            }
            dbPool.closeConn(conn);
            cacheService->updateSinhVien(sinhVien->MaSv, sinhVien->Mac);
            return response.build(200, "Sinh Vien " + sinhVien->FullName + " diem danh thanh cong", sinhVien);
        } catch (...) {
            dbPool.closeConn(conn);
            return response.build(500, "UN_DETECTED_EROR", sinhVien);
        }
    });
}

SinhVien CheckInService::CheckSinhVien(shared_ptr<DBConnection> &conn, const string &maSv) {
    return repo.KiemTra(conn, querry.GetQuerry(Querry::querryType::KiemTraSinhVien, maSv));
}

bool CheckInService::CheckInSinhVien(shared_ptr<DBConnection> &conn, const string &maSv, string &macAdress) {
    return repo.DiemDanh(conn, maSv, macAdress);
}

vector<struct DiemDanh> CheckInService::GetAllSinhVien(const vector<string> &className) {
    vector<struct DiemDanh> *dsSinhVien = new vector<struct DiemDanh>();
    auto conn = dbPool.getConn();
    if (!conn || !conn->get()) { return *dsSinhVien; }

    try {
        auto f = repo.GetAllSinhVien(conn, className);
        *dsSinhVien = f;
        dbPool.closeConn(conn);
        return *dsSinhVien;
    } catch (...) {
        dbPool.closeConn(conn);
        return vector<struct DiemDanh>();
    }
}
