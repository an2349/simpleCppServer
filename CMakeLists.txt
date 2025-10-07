cmake_minimum_required(VERSION 4.0)
project(LTHTM)

set(CMAKE_CXX_STANDARD 20)

add_executable(LTHTM #../Server.cpp
        Server.cpp
        configs.h
        Controller/Controller.h
        Controller/Controller.cpp
        Model/SinhVien.cpp
        Model/SinhVien.h
        Model/Response.cpp
        Model/Response.h
        Services/CheckInService.cpp
        Services/CheckInService.h
        Repo/DbPool.cpp
        Repo/DbPool.h
        Repo/Querry.cpp
        Repo/Querry.h
        Repo/PoolManager.h
        Repo/SinhVienRepo.cpp
        Repo/SinhVienRepo.h
        Model/ContentModel.cpp
        Model/ContentModel.h
        Model/MultiPartModel.cpp
        Model/MultiPartModel.h
        Services/FileUpLoadServices.cpp
        Services/FileUpLoadServices.h
        Model/RequestModel.cpp
        Model/RequestModel.h
        Model/Cache.cpp
        Model/Cache.h
        Services/CacheService.cpp
        Services/CacheService.h)
target_link_libraries(LTHTM PRIVATE /usr/lib/x86_64-linux-gnu/libmysqlcppconn.so)
