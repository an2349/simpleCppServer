//
// Created by an on 10/5/25.
//

#ifndef LTHTM_FILEUPLOADSERVICES_H
#define LTHTM_FILEUPLOADSERVICES_H
#include <future>
#include <string>
#include <filesystem>
#include "../Model/MultiPartModel.h"
#include "../Model/Response.h"

using namespace std;
namespace fs = std::filesystem;

class FileUpLoadServices {
private:
    string filePath = "../uploads";
    mutex mtx;

    struct FileStatus {
        bool isUpload = false;
        bool isSave = false;
    };

public:
    FileUpLoadServices() {
        if (!fs::exists(filePath)) {
            fs::create_directories(filePath);
        }
    }

    future<string> UpLoadAsync(MultiPartModel *multipart, const string &boundary, const string& clientIp, const string& clientId);

    FileStatus SaveFileAsync(MultiPartModel *multipart, const string &boundary, const string& clientIp, const string& clientId);
};


#endif //LTHTM_FILEUPLOADSERVICES_H
