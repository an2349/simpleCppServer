//
// Created by an on 10/5/25.
//

#ifndef LTHTM_MULTIPARTMODEL_H
#define LTHTM_MULTIPARTMODEL_H
#include <cstdint>
#include <vector>
#include <string>
using namespace std;

class MultiPartModel {
public:
    MultiPartModel(){}
    int      totalPart    = 1;
    string   name         ="";
    int      part         = 1;
    string   contentType   = "";
    size_t   totalSize     = 0;
    vector<uint8_t>  value   ;
    MultiPartModel bindMultiPart(const string& body, const string& boundary);
    static vector<MultiPartModel> bindMultiParts(const string& body, const string& boundary) {
        vector<MultiPartModel> parts;
        string delimiter = "--" + boundary;
        size_t start = 0;

        while (true) {
            size_t partStart = body.find(delimiter, start);
            if (partStart == string::npos) break;

            partStart += delimiter.size();

            if (body.substr(partStart, 2) == "\r\n") partStart += 2;

            size_t partEnd = body.find(delimiter, partStart);
            if (partEnd == string::npos) partEnd = body.size();

            string partData = body.substr(partStart, partEnd - partStart);

            if (partData.find("--") == 0) {
                break;
            }
            if (partData.empty()) break;

            MultiPartModel part = MultiPartModel().bindMultiPart(partData, boundary);
            parts.push_back(part);

            start = partEnd;
        }

        return parts;
    }

};
inline string trim(const string& s) {
    size_t start = s.find_first_not_of("\r\n ");
    size_t end = s.find_last_not_of("\r\n ");
    if (start == string::npos || end == string::npos) return "";
    return s.substr(start, end - start + 1);
}


#endif //LTHTM_MULTIPARTMODEL_H