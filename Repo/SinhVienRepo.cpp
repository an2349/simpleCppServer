#include "SinhVienRepo.h"
#include "DbPool.h"
//#include "../Model/Cache.h"

bool SinhVienRepo::DiemDanh(const shared_ptr<DBConnection>& conn, const string& maSv, const string& macAdress) {
    if (!conn || !conn->get()) return false;
    try {
        unique_ptr<sql::PreparedStatement> sql(
            conn->get()->prepareStatement("CALL proc_diem_danh_tong_hop(?, ?)")
        );
        sql->setString(1, maSv);
        sql->setString(2, macAdress);
        sql->executeUpdate();
        while (sql->getMoreResults()) {
            unique_ptr<sql::ResultSet> trash(sql->getResultSet());
        }
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



vector<struct DiemDanh> SinhVienRepo::GetAllSinhVien(const shared_ptr<DBConnection>& conn, const vector<string>& className) {
    vector<struct DiemDanh> dsSinhVien;
    if (!conn || !conn->get()) return dsSinhVien;
    try {
        string joinedClasses;
        for (size_t i = 0; i < className.size(); ++i) {
            joinedClasses += className[i];
            if (i != className.size() - 1) joinedClasses += ",";
        }
        unique_ptr<sql::PreparedStatement> sql(
            conn->get()->prepareStatement("CALL proc_get_cache(?);"));

        sql->setString(1, joinedClasses);
        unique_ptr<sql::ResultSet> res(sql->executeQuery());

        while (res->next()) {
            struct DiemDanh* diemDanh = new struct DiemDanh();
            diemDanh->IsCheckIn = res->getBoolean("dd");
            diemDanh->Mac = res->getString("mac");
            diemDanh->MaSv = res->getString("user_name");
            diemDanh->FullName = res->getString("full_name");
            diemDanh->ClassName = res -> getString("class_name");
            dsSinhVien.push_back(*diemDanh);
            delete diemDanh;
        }
        while (sql->getMoreResults()) {
            unique_ptr<sql::ResultSet> trash(sql->getResultSet());
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