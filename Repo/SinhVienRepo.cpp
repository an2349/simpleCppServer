#include "SinhVienRepo.h"
#include "DbPool.h"
//#include "../Model/Cache.h"

bool SinhVienRepo::DiemDanh(const shared_ptr<DBConnection>& conn, const string& query) {
    if (!conn || !conn->get()) return false;
    try {
        unique_ptr<sql::PreparedStatement> sql(
            conn->get()->prepareStatement(query)
        );
        sql->execute();
        return true;
    } catch (sql::SQLException& e) {
        cerr << "MySQL Error: " << e.what() << "\n";
        return false;
    }
}

SinhVien SinhVienRepo::KiemTra(const shared_ptr<DBConnection>& conn, const string& query) {
    SinhVien sinhVien ;//= new sinhVien();
    if (!conn || !conn->get()) return sinhVien;
    try {
        unique_ptr<sql::PreparedStatement> sql(
            conn->get()->prepareStatement(query)
        );
        unique_ptr<sql::ResultSet> res(sql->executeQuery());

        if (res->next()) {
            sinhVien.MaSv      =       res->getString("user_name");
            sinhVien.ClassName =       res->getString("class_name");
            sinhVien.Date      =       res->getString("modified_date");
            sinhVien.FullName  =       res->getString("full_name");
            sinhVien.Mac          =       res->getString("mac");
            return sinhVien;
        }
        else {
            sinhVien.MaSv = "";
            sinhVien.ClassName = "";
            return sinhVien;
        }
    }
    catch (sql::SQLException& e) {
        cerr << "MySQL Error: " << e.what() << "\n";
        return sinhVien;
    }
    catch (...) {
        cerr << "MySQL Error: Unknown\n";
        return sinhVien;
    }
}
vector<struct DiemDanh> SinhVienRepo::GetAllSinhVien(const shared_ptr<DBConnection>& conn,const string& className) {
    vector<struct DiemDanh> dsSinhVien;
    if (!conn || !conn->get()) return dsSinhVien;
    try {
        unique_ptr<sql::PreparedStatement> sql(
            conn->get()->prepareStatement("CALL proc_get_cache(?)"));

        sql->setString(1, className);
        unique_ptr<sql::ResultSet> res(sql->executeQuery());

        while (res->next()) {
            struct DiemDanh diemDanh;
            diemDanh.IsCheckIn = res->getBoolean("dd");
            diemDanh.Mac = res->getString("mac");
            diemDanh.MaSv = res->getString("user_name");
            diemDanh.FullName = res->getString("full_name");
            dsSinhVien.push_back(diemDanh);
        }
        return dsSinhVien;
    }
    catch (sql::SQLException& e) {
        cerr << "MySQL Error: " << e.what() << "\n";
        return dsSinhVien;
    }
    catch (...) {
        cerr << "MySQL Error: Unknown\n";
        return dsSinhVien;
    }
}
