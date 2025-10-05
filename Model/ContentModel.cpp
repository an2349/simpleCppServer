//
// Created by an on 10/5/25.
//

#include "ContentModel.h"
ContentModel ContentModel::parseJson(const string& body) {
    try {
        json js = json::parse(body);
        return  js.get<ContentModel>();
    }
    catch (...) {
        return ContentModel();
    }
}