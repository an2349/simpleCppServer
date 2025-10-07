//
// Created by an on 10/7/25.
//

#include "RequestModel.h"
RequestModel RequestModel::parseJson(const string& body) {
    try {
        json js = json::parse(body);
        return  js.get<RequestModel>();
    }
    catch (...) {
        return RequestModel();
    }
}