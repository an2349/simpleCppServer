//
// Created by an on 10/5/25.
//

#ifndef LTHTM_FILEUPLOADSERVICES_H
#define LTHTM_FILEUPLOADSERVICES_H
#include <future>
#include <string>
#include <filesystem>
#include "../Model/MultiPartModel.h"
using namespace std;
namespace fs = std::filesystem;

class FileUpLoadServices {
private:
    string filePath ="../uploads";
    mutex mtx;
public://FileUpLoadServices(){}
    FileUpLoadServices() {
        if (!fs::exists(filePath)) {
            fs::create_directories(filePath);}
    }
    future<bool> UpLoadAsync(MultiPartModel* multipart,const string& boundary);
    future<bool> SaveFileAsync(MultiPartModel* multipart,const string& boundary);
};


#endif //LTHTM_FILEUPLOADSERVICES_H