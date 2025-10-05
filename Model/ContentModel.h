//
// Created by an on 10/5/25.
//

#ifndef LTHTM_CONTENTMODEL_H
#define LTHTM_CONTENTMODEL_H
#include <string>
#include <nlohmann/json.hpp>
using namespace  std;
using nlohmann::json;
class ContentModel {
public:
    ContentModel(){}
    int      totalPart   = 1;
    string   name        = "file";
    int      part        = 1;
    string   value       ="";
    ContentModel parseJson(const string& body);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(ContentModel, totalPart, name, part, value);
};
#endif //LTHTM_CONTENTMODEL_H