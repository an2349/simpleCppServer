//
// Created by an on 10/7/25.
//

#include "Cache.h"
unordered_map<string, DiemDanh> Cache::danhSachSV;
shared_mutex Cache::cacheMutex;