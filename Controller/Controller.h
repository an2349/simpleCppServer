//
// Created by an on 10/4/25.
//

#ifndef LTHTM_ISERVICES_H
#define LTHTM_ISERVICES_H
#pragma once
#include <string>
#include <future>
#include "../Model/Response.cpp"
#include  "../Model/MultiPartModel.h"
#include "../Model/RequestModel.h"
#include "../Services/CheckInService.h"
#include "../Services/FileUpLoadServices.h"
using namespace std;

class Controller {
public:
    Controller(CheckInService& checkInServices, FileUpLoadServices& fileUpLoadServices)
    : checkInService(checkInServices), fileService(fileUpLoadServices){}

    string handleRequestAsync(vector<char>* req,const string& clientMAC, const string& clientIp);

private:
    CheckInService& checkInService;
    FileUpLoadServices& fileService;

    enum class methods{ POST ,PUT,NOT };
    string GetMethod(const string& req );
    methods CheckMethod(const string& req);

    string GetURL(const string& req);
    string GetBody(const string& req);

    string GetMsv(const string& body);
    bool Validate(const string& maSv);

    string GetContentType(const string& req);
    bool CheckConTent(const string& contentType);

    string GetBoundary(const string& body);
};

#endif //LTHTM_ISERVICES_H