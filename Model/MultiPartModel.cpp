//
// Created by an on 10/5/25.
//

#include "MultiPartModel.h"

#include <sstream>
MultiPartModel MultiPartModel::bindMultiPart(const string& httpBody, const string& boundary) {
    MultiPartModel model;

    string partData = trim(httpBody);
    if (partData.empty()) return model;

    size_t headerEnd = partData.find("\r\n\r\n");
    if (headerEnd == string::npos) return model;

    string headers = partData.substr(0, headerEnd);
    string bodyRaw = partData.substr(headerEnd + 4);
    bodyRaw = trim(bodyRaw);

    istringstream headerStream(headers);
    string line;
    while (getline(headerStream, line)) {
        line = trim(line);

        if (line.find("Content-Disposition:") != string::npos) {
            size_t fnamePos = line.find("filename=\"");
            if (fnamePos != string::npos) {
                fnamePos += 10;
                size_t fnameEnd = line.find("\"", fnamePos);
                model.name = line.substr(fnamePos, fnameEnd - fnamePos);
            }

            size_t partPos = line.find("part=\"");
            if (partPos != string::npos) {
                partPos += 6;
                size_t partEnd = line.find("\"", partPos);
                model.part = stoi(trim(line.substr(partPos, partEnd - partPos)));
            }

            size_t totalPos = line.find("total=\"");
            if (totalPos != string::npos) {
                totalPos += 7;
                size_t totalEnd = line.find("\"", totalPos);
                model.totalPart = stoi(trim(line.substr(totalPos, totalEnd - totalPos)));
            }
            size_t sizePos = line.find("size=\"");
            if (sizePos != string::npos) {
                sizePos += 6;
                size_t sizeEnd = line.find("\"", sizePos);
                model.totalSize = stoi(trim(line.substr(sizePos, sizeEnd - sizePos)));
            }
        }
        else if (line.find("Content-Type:") != string::npos) {
            size_t ctPos = line.find(":");
            if (ctPos != string::npos) {
                model.contentType = trim(line.substr(ctPos + 1));
            }
        }
    }

    model.value = vector<uint8_t>(bodyRaw.begin(), bodyRaw.end());
    return model;
}
