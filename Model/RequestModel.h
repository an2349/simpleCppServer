//
// Created by an on 10/7/25.
//

#ifndef LTHTM_REQUESTMODEL_H
#define LTHTM_REQUESTMODEL_H
#include <string>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;
class RequestModel {
public:
    RequestModel(){}
    string  MaSv        = "";
    string  Mac         = "";
    RequestModel parseJson(const string& body);

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(RequestModel, MaSv, Mac);
};


#endif //LTHTM_REQUESTMODEL_H