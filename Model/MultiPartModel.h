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
    static MultiPartModel bindMultiPart(vector<char>* body, const string& boundary);
    static vector<MultiPartModel> bindMultiParts(vector<char>* request, const string& boundary) {
        vector<MultiPartModel> parts;
        string delimiter = "--" + boundary;
        size_t start = 0;

        auto findDelimiter = [&](size_t pos) -> size_t {
            for (size_t i = pos; i + delimiter.size() <= request->size(); ++i) {
                bool match = true;
                for (size_t j = 0; j < delimiter.size(); ++j) {
                    if ((*request)[i + j] != delimiter[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) return i;
            }
            return string::npos;
        };

        while (true) {
            size_t partStart = findDelimiter(start);
            if (partStart == string::npos) break;

            partStart += delimiter.size();
            if (partStart + 2 <= request->size() &&
                (*request)[partStart] == '\r' &&
                (*request)[partStart + 1] == '\n') {
                partStart += 2;
                }

            size_t partEnd = findDelimiter(partStart);
            if (partEnd == string::npos) partEnd = request->size();

            if (partEnd <= partStart) break;

            vector<char>* partData= new vector<char>(request->begin() + partStart, request->begin() + partEnd);

            if (partData->size() >= 2 && (*partData)[0] == '-' && (*partData)[1] == '-') break;

            MultiPartModel part = bindMultiPart(partData, boundary);
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