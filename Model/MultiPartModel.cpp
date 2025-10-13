//
// Created by an on 10/5/25.
//

#include "MultiPartModel.h"

#include <algorithm>
#include <sstream>
MultiPartModel* MultiPartModel::bindMultiPart(vector<char>* request, const string& boundary) {
    MultiPartModel* model = new MultiPartModel();

    if (!request || request->empty()) return model;

    auto it = std::search(request->begin(), request->end(),
                          "\r\n\r\n", "\r\n\r\n" + 4); // iterator đến cuối header
    if (it == request->end()) return model;

    string headers(request->begin(), it);
    headers = trim(headers);

    auto bodyStartIt = it + 4;
    string bodyRaw(bodyStartIt, request->end());
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
                model->name = line.substr(fnamePos, fnameEnd - fnamePos);
            }

            size_t partPos = line.find("part=\"");
            if (partPos != string::npos) {
                partPos += 6;
                size_t partEnd = line.find("\"", partPos);
                model->part = stoi(trim(line.substr(partPos, partEnd - partPos)));
            }

            size_t totalPos = line.find("total=\"");
            if (totalPos != string::npos) {
                totalPos += 7;
                size_t totalEnd = line.find("\"", totalPos);
                model->totalPart = stoi(trim(line.substr(totalPos, totalEnd - totalPos)));
            }

            size_t sizePos = line.find("size=\"");
            if (sizePos != string::npos) {
                sizePos += 6;
                size_t sizeEnd = line.find("\"", sizePos);
                model->totalSize = stoi(trim(line.substr(sizePos, sizeEnd - sizePos)));
            }
        }
        else if (line.find("Content-Type:") != string::npos) {
            size_t ctPos = line.find(":");
            if (ctPos != string::npos) {
                model->contentType = trim(line.substr(ctPos + 1));
            }
        }
    }

    model->value = vector<uint8_t>(bodyRaw.begin(), bodyRaw.end());

    return model;
}