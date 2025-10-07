//
// Created by an on 10/7/25.
//

#include "CacheService.h"
void CacheService::loadCache(const string& className) {
    auto allSV = checkInService.GetAllSinhVien(className).get();
    unique_lock lock(cache.cacheMutex);
    for (const auto& sv : allSV) {
        cache.danhSachSV[sv.MaSv] = DiemDanh{false, sv.MaSv, sv.Mac};
    }
}
future<DiemDanh> CacheService::checkSinhVien(const string& maSv,const string& macAdress) {
    return async(launch::async, [this, maSv, macAdress]() -> DiemDanh {
        shared_lock lock(cache.cacheMutex);
        auto it = cache.danhSachSV.find(maSv);
        if (it != cache.danhSachSV.end()) {
            DiemDanh &dd = it->second;
            return dd;
        }
        return DiemDanh{false, "",""};
    });
}
void CacheService::updateSinhVien(const string& maSv, const string& macAdress) {
    unique_lock lock(cache.cacheMutex);
    auto it = cache.danhSachSV.find(maSv);
    if (it != cache.danhSachSV.end()) {
        it->second.IsCheckIn = true;
        it->second.Mac = macAdress;
    }
}

