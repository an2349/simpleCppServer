//
// Created by an on 10/7/25.
//

#include "CacheService.h"

void CacheService::loadCache(const vector<string> &className) {
    auto allSV = checkInService.GetAllSinhVien(className);
    unique_lock lock(cache.cacheMutex);
    for (const auto &sv: allSV) {
        cache.danhSachSV[sv.MaSv] = DiemDanh{sv.IsCheckIn, sv.MaSv, sv.Mac, sv.FullName, sv.ClassName};
        cache.svLocks[sv.MaSv];
    }
}

DiemDanh CacheService::checkSinhVien(const string &maSv, const string &macAdress) {
    shared_lock lock(cache.cacheMutex);
    auto it = cache.danhSachSV.find(maSv);
    if (it != cache.danhSachSV.end()) {
        DiemDanh &dd = it->second;
        return dd;
    }
    return DiemDanh{false, "", ""};
}

DiemDanh CacheService::checkSinhVien(const string &macAdress) {
    shared_lock lock(cache.cacheMutex);
    //auto it = cache.danhSachSV.find(macAdress);
    for (const auto &[maSv, dd]: cache.danhSachSV) {
        if (dd.Mac == macAdress) {
            return dd;
        }
    }
    return DiemDanh{false, "", ""};
}

void CacheService::updateSinhVien(const string &maSv, const string &macAdress) {
    shared_lock mapLock(cache.cacheMutex);
    auto it = cache.danhSachSV.find(maSv);
    if (it == cache.danhSachSV.end()) return;
    mapLock.unlock();
    lock_guard lock(cache.svLocks[maSv]);
    it->second.IsCheckIn = true;
    it->second.Mac = macAdress;
}
