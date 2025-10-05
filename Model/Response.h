//
// Created by an on 10/4/25.
//

#ifndef LTHTM_RESPONSE_H
#define LTHTM_RESPONSE_H


//
// Created by an on 10/4/25.
//
#include <string>
#include <nlohmann/json.hpp>
using  namespace  std;
template<typename T>
class Response {
public:
    Response(){}
    string Message;
    string Code;
    string Data;
    //string build(const int& statusCode,const string& Message,T* Body);
    //template<typename T>
    string build(const int& statusCode,const string& Message,T* Body) {
        stringstream ss;
        Response<string> respon;
        nlohmann::json js = *Body;
        string BodyJson = js.dump();
        delete Body;

        respon.Code = to_string(statusCode);
        respon.Data = BodyJson;
        respon.Message = Message;

        nlohmann::json j = {
            {"Code", respon.Code},
            {"Message", respon.Message},
            {"Data", respon.Data}
        };
        string responJson = j.dump();

        ss << "HTTP/1.1" << " " << statusCode << "\r\n"
           << "Content-Type: " << "application/json" << "\r\n"
           << "Content-Length: " << responJson.length() << "\r\n\r\n"
           <<responJson<< "\r\n";
        return ss.str();
    }
};
#endif //LTHTM_RESPONSE_H