//
// Created by an on 10/4/25.
//
#include "Controller.h"
#include <iostream>
#include <regex>
using namespace std;

future<string> Controller::handleRequestAsync(const string& req) {
    return async(launch::async, [req, this]() {
        Response<string> response;
        try {
            //cout << "Nhận request: " << req << "\n";
            size_t hEnd = req.find("\r\n\r\n");
            if(hEnd == string::npos) {
                return response.build(400,"Form không hợp lệ",new string(""));
            }
            string method = GetMethod(req);
            methods checkedMethod = CheckMethod(method);
            if(checkedMethod == methods::NOT) {
                return response.build(400, "Phương thức khôgn hợp lệ",new string(""));
            }

            string url  = GetURL(req);
            if (url == "") {
                return response.build(400,"API không hợp lệ",new string(""));
            }

            string body = GetBody(req);
            if(body == "") {
                return response.build(400,"body khôgn hợp lệ",new string(""));
            }

            if (url == "/maso" && checkedMethod == methods::POST) {
                RequestModel* requestModel = new RequestModel();
                *requestModel = requestModel-> parseJson(body);
                string maSv = requestModel->MaSv.empty() ? "" : requestModel->MaSv;
                string macAdress = requestModel->Mac.empty() ? "" : requestModel->Mac;
                delete requestModel;
                if (maSv == "" || macAdress == "") {
                    return response.build(400,"khong hop le",new string(""));
                }
                return checkInService.CheckInAsync(maSv, macAdress).get();
            }

            if (url == "/upload" && checkedMethod == methods::PUT)
                {
                string boundary = GetBoundary(GetContentType(req));
                vector<MultiPartModel>* parts = new vector<MultiPartModel>();
                *parts = MultiPartModel::bindMultiParts(body, boundary);
                //cout<<"boundary la "+ boundary<<"\n"<<parts->size()<<"\n";
                if (parts->empty()) {
                    delete parts;
                    return response.build(400, "Khong hop le1", new string(""));
                }
                vector<future<bool>> futures;
                for (auto &part : *parts)
                    {
                    if (part.part <= 0 || part.totalPart <= 0 || part.value.empty())
                        {
                        delete parts;
                        return response.build(400, "Khong hop le2", new string(""));
                        }
                    futures.push_back(fileService.UpLoadAsync(&part, boundary));
                    }
                bool success = true;
                for (auto &fut : futures) {
                    if (!fut.get()) {
                        success = false;
                        break;
                    }
                }
                if (!success) {
                    delete parts;
                    return response.build(300, "Upload that bai", new string(""));
                }
                delete parts;
                return response.build(200, "Upload thanh cong", new string(""));
            }

            /*
            if (url == "/upload" && checkedMethod ==methods::PUT) {
                ContentModel* contentModel = new ContentModel();
                *contentModel = contentModel->parseJson(body);
                cout<<body<<"\n";
                cout<<contentModel->name;
                if (contentModel->name == ""
                    || contentModel->value == ""
                    || contentModel->totalPart <=0
                    || contentModel->part <=0)
                    {
                    delete contentModel;
                    return response.build(400 ,"Khong hop le",new string(""));
                    }
                return response.build(200 ,"Đã nhận PUT, trả về thông báo OK",new string(""));
            }*/

            //if (url == "/upload-many" && checkedMethod == methods::PUT) {
            //    return response.build(400 ,"nothing",new string(""));
            //}

            else {
                return response.build(400 ,"Yêu cầu không hợp lệ",new string(""));
            }
        }
        catch (exception& e){
            cerr<<e.what()<<endl;
            return response.build(500 ,"Internal Server Error",new string(""));
        }
        catch (...) {
            cerr << "Unknown exception\n";
            return response.build(500,"Internal Server Error",new string(""));
        }
    });
}



Controller::methods Controller::CheckMethod(const string& method) {
    if (method == "POST" ) return methods::POST;
    if (method == "PUT")  return methods::PUT;
    return methods::NOT;
}



string Controller::GetMethod(const string& req) {
    size_t pos = req.find(' ');
    if(pos == string::npos) {
        return "";
    }
    return req.substr(0, pos);
}



string Controller::GetURL(const string& req) {
    size_t space1 = req.find(' ');
    if(space1 == string::npos) {
        return "";
    }
    size_t space2 = req.find(' ', space1 + 1);
    if(space2 == string::npos) {
        return "";
    }
    return req.substr(space1 + 1, space2 - space1 - 1);
}



string Controller::GetBody(const string& req) {
    size_t startB = req.find("\r\n\r\n");
    if(startB == string::npos) {
        return "";
    }
    return req.substr(startB + 4, req.size() - startB - 4);
}



string Controller::GetMsv(const string& body) {
    try {
        size_t dot1 = body.find('"');
        if(dot1 == string::npos) {
            return "";
        }
        size_t dot2 = body.find('"', dot1 + 1);
        if(dot2 == string::npos) {
            return "";
        }
        string maSv = body.substr(dot1 + 1, dot2 - dot1 - 1);
        bool success = CheckMsv(maSv);
        if(success) {
            return maSv;
        }
        else return "";
    }
    catch (...) {
        return "";
    }
}



bool Controller::CheckMsv(const string& maSv) {
    if (maSv.empty()) return false;

    regex pattern("^[a-zA-Z0-9_-]+$");
    if (!regex_match(maSv, pattern)) return false;

    const string forbidden = ";'\"--/**/";
    for (char c : maSv) {
        if (forbidden.find(c) != string::npos) return false;
    }

    return true;
}


string Controller::GetContentType(const string& req) {
    string result;
    string key = "Content-Type:";
    size_t pos = req.find(key);
    if (pos == string::npos) return result;

    pos += key.length();
    while (pos < req.size() && (req[pos] == ' ' || req[pos] == '\t')) pos++;

    size_t end = req.find("\r\n", pos);
    if (end == string::npos) end = req.size();

    result = req.substr(pos, end - pos);
    return result;
}


string Controller::GetBoundary(const string& contentType) {
    string key = "boundary=";
    size_t pos = contentType.find(key);
    if (pos == string::npos) return "";
    pos += key.size();
    if (contentType[pos] == '\"') {
        pos++;
        size_t end = contentType.find('\"', pos);
        if (end == string::npos) return ""; // lỗi
        return contentType.substr(pos, end - pos);
    } else {
        size_t end = contentType.find(';', pos);
        if (end == string::npos) end = contentType.size();
        return contentType.substr(pos, end - pos);
    }
}
