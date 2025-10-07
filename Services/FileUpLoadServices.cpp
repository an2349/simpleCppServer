//
// Created by an on 10/5/25.
//

#include "FileUpLoadServices.h"

#include <fstream>
#include <iostream>

future<bool> FileUpLoadServices::UpLoadAsync(MultiPartModel* multipart, const string& boundary) {
    return async(launch::async, [multipart, boundary,this]() {
        return SaveFileAsync(multipart, boundary).get();
    });
}
future<bool> FileUpLoadServices::SaveFileAsync(MultiPartModel* multipart, const string& boundary) {
        return async(launch::async, [multipart, boundary,this]() {
            try {
                lock_guard<mutex> lock(mtx);

                string boundaryFolder = filePath + "/" + boundary;
                if (!fs::exists(boundaryFolder)) {
                    fs::create_directories(boundaryFolder);
                }

                string chunkFile = boundaryFolder + "/" + to_string(multipart->part) + ".chunk";
                ofstream ofs(chunkFile, ios::binary);
                if (!ofs.is_open()) {std::cout<<"loi1";return false;}

                ofs.write((char*)multipart->value.data(), multipart->value.size());
                ofs.close();

                bool allChunksPresent = true;
                for (int i = 1; i <= multipart->totalPart; i++) {
                    string path = boundaryFolder + "/" + to_string(i) + ".chunk";
                    if (!fs::exists(path)) {
                        allChunksPresent = false;
                        break;
                    }
                }
                if (allChunksPresent) {
                    string finalFile = filePath + "/" + multipart->name;
                    ofstream ofsFinal(finalFile, ios::binary);
                    if (!ofsFinal.is_open()) {std::cout<<"loi2";return false;}

                    for (int i = 1; i <= multipart->totalPart; i++) {
                        string path = boundaryFolder + "/" + to_string(i) + ".chunk";
                        ifstream ifs(path, ios::binary);
                        ofsFinal << ifs.rdbuf();
                        ifs.close();
                    }
                    ofsFinal.close();
                    fs::remove_all(boundaryFolder);
                }
                //fs::remove_all(boundaryFolder);
                return true;
            }
            catch (exception& e) {
                std::cerr << e.what() << std::endl;
                return false;
            }
            catch (...) {
                std::cout<<"loi o save file";
                return false;
            }
        });
}