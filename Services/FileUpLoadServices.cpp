//
// Created by an on 10/5/25.
//

#include "FileUpLoadServices.h"

#include <fstream>
#include <iostream>


future<string> FileUpLoadServices::UpLoadAsync(MultiPartModel *multipart, const string &boundary) {
    return async(launch::async, [multipart, boundary,this]() {
        Response<MultiPartModel> response;
        try {
            FileStatus fileStatus = SaveFileAsync(multipart, boundary);
            if (fileStatus.isUpload == true) {
                if (fileStatus.isSave == false) {
                    return response.build(200, "Nhan ok", multipart);
                }
                return response.build(200, "Upload thanh cong", multipart);
            }
            return response.build(400, "Upload that bai", multipart);
        } catch (exception &e) {
            cout << e.what() << endl;
            return response.build(400, "Upload that bai", multipart);
        }
    });
}

FileUpLoadServices::FileStatus FileUpLoadServices::SaveFileAsync(MultiPartModel *multipart, const string &boundary) {
    FileStatus fileStatus = FileStatus();
    try {
        lock_guard<mutex> lock(mtx);

        string boundaryFolder = filePath + "/" + boundary;
        if (!fs::exists(boundaryFolder)) {
            fs::create_directories(boundaryFolder);
        }

        string chunkFile = boundaryFolder + "/" + to_string(multipart->part) + ".chunk";
        ofstream ofs(chunkFile, ios::binary);
        if (!ofs.is_open()) {
            std::cout << "loi save chunk";
            return fileStatus;
        }

        ofs.write((char *) multipart->value.data(), multipart->value.size());
        ofs.close();
        fileStatus.isUpload = true;
        //ghep file
        bool allChunksPresent = true;
        for (int i = 1; i <= multipart->totalPart; i++) {
            string path = boundaryFolder + "/" + to_string(i) + ".chunk";
            if (!fs::exists(path)) {
                allChunksPresent = false;
                break;
            }
        }
        if (allChunksPresent) {
            string timestamp = to_string(time(nullptr));
            string finalFile = filePath + "/" + multipart->name + "_" + timestamp;
            ofstream ofsFinal(finalFile, ios::binary);
            if (!ofsFinal.is_open()) {
                std::cout << "loi ghep chunk";
                return fileStatus;
            }

            for (int i = 1; i <= multipart->totalPart; i++) {
                string path = boundaryFolder + "/" + to_string(i) + ".chunk";
                ifstream ifs(path, ios::binary);
                ofsFinal << ifs.rdbuf();
                ifs.close();
            }
            ofsFinal.close();
            fileStatus.isSave = true;
            fs::remove_all(boundaryFolder);
        }
        //fs::remove_all(boundaryFolder);
        return fileStatus;
    } catch (exception &e) {
        std::cerr << e.what() << std::endl;
        return fileStatus;
    } catch (...) {
        std::cout << "loi o save file";
        return fileStatus;
    }
}
