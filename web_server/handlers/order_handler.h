#ifndef USEHANDLER_H
#define USEHANDLER_H

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTMLForm.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>

using Poco::DateTimeFormat;
using Poco::DateTimeFormatter;
using Poco::ThreadPool;
using Poco::Timestamp;
using Poco::Net::HTMLForm;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::NameValueCollection;
using Poco::Net::ServerSocket;
using Poco::Util::Application;
using Poco::Util::HelpFormatter;
using Poco::Util::Option;
using Poco::Util::OptionCallback;
using Poco::Util::OptionSet;
using Poco::Util::ServerApplication;

#include "../../database/order.h"


static bool hasSubstr(const std::string &str, const std::string &substr)
{
    if (str.size() < substr.size())
        return false;
    for (size_t i = 0; i <= str.size() - substr.size(); ++i)
    {
        bool ok{true};
        for (size_t j = 0; ok && (j < substr.size()); ++j)
            ok = (str[i + j] == substr[j]);
        if (ok)
            return true;
    }
    return false;
}

class ServiceHandler : public HTTPRequestHandler
{
private:
    bool check_name(const std::string &status, std::string &reason)
    {
        if (status.length() < 3)
        {
            reason = "Name must be at leas 3 signs";
            return false;
        }
        return true;
    };

    bool check_price(const std::string &price, std::string &reason)
    {     
        if (stod(price) < 0)
        {
            reason = "Price must be non-negative";
            return false;
        }
        return true;
    };

    void badRequestError(HTTPServerResponse &response,  std::string instance)
    {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_BAD_REQUEST);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
        root->set("content", "/errors/bad_request");
        root->set("title", "Internal exception");
        root->set("status", "400");
        root->set("detail", "Недостаточно параметров в теле запроса");
        root->set("instance", instance);
        std::ostream &ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }

    void notFoundError(HTTPServerResponse &response, std::string instance, std::string message)
    {
        response.setStatus(Poco::Net::HTTPResponse::HTTP_NOT_FOUND);
        response.setChunkedTransferEncoding(true);
        response.setContentType("application/json");
        Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
        root->set("content", "/errors/not_found");
        root->set("title", "Internal exception");
        root->set("status", "404");
        root->set("detail", message);
        root->set("instance", instance);
        std::ostream &ostr = response.send();
        Poco::JSON::Stringifier::stringify(root, ostr);
    }
    

public:
    ServiceHandler(const std::string &format) : _format(format)
    {
    }

    void handleRequest(HTTPServerRequest &request,
                       HTTPServerResponse &response)
    {
        HTMLForm form(request, request.stream());
        try
        {
            response.set("Access-Control-Allow-Origin", "*"); //для работы swagger
            if (hasSubstr(request.getURI(), "/orders"))
            {
                if(form.has("client_id"))
                {
                    long client_id = atol(form.get("client_id").c_str());
                    
                    auto results = database::Order::read_by_client_id(client_id);
                    if(!results.empty())
                    {
                        Poco::JSON::Array arr;
                        for (auto s : results)
                            arr.add(s.toJSON());
                        
                        response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                        response.setChunkedTransferEncoding(true);
                        response.setContentType("application/json");
                        std::ostream &ostr = response.send();
                        Poco::JSON::Stringifier::stringify(arr, ostr);

                        return;
                    }
                    else
                    {
                        notFoundError(response, request.getURI(), "Orders not found");
                        return;
                    }                   
                }
                else
                {
                    badRequestError(response, request.getURI());
                    return;
                }
            }         
            else if (hasSubstr(request.getURI(), "/order"))
            {
                if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_GET)
                {
                    if(form.has("id"))
                    {
                        long id = atol(form.get("id").c_str());
                        std::optional<database::Order> result = database::Order::read_by_id(id);
                        if (result)
                        {               
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            std::ostream &ostr = response.send();
                            Poco::JSON::Stringifier::stringify(result->toJSON(), ostr);
                            
                            return;
                        }
                        else
                        {
                            notFoundError(response, request.getURI(), "Order id " + std::to_string(id) + " not found");
                            return;
                        }
                    }
                    else
                    {
                        badRequestError(response, request.getURI());
                        return;
                    }
                    
                }
                else if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_POST)
                {
                    if (form.has("status") && form.has("client_id") && form.has("service_id"))
                    {
                        database::Order service;
                        service.status() = form.get("status");
                        if(form.has("content"))
                            service.content() = form.get("content");
                        //add check foreign key
                        service.client_id() = atol(form.get("client_id").c_str());
                        service.service_id() = atol(form.get("service_id").c_str());
    
    
                        if (service.save_to_mysql())
                        {                   
                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                            root->set("created", service.id());
                            std::ostream &ostr = response.send();
                            Poco::JSON::Stringifier::stringify(root, ostr);

                            return;
                        }
                        else
                        {
                            notFoundError(response, request.getURI(), "Adding an order failed");
                            return;
                        }
                    }
                    else
                    {
                        badRequestError(response, request.getURI());
                        return;
                    }
                }
                else if(request.getMethod() == Poco::Net::HTTPRequest::HTTP_PUT)
                {
                    if(form.has("id") && (form.has("status") || form.has("content")) )
                    {
                        long id = atol(form.get("id").c_str());
                        
                        std::optional<database::Order> service = database::Order::read_by_id(id);
                        if(service)
                        {
                            if(form.has("status"))
                                service->status() = form.get("status");
                            if(form.has("content"))
                                service->content() = form.get("content");

                             service->update_in_mysql();

                            response.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                            response.setChunkedTransferEncoding(true);
                            response.setContentType("application/json");
                            Poco::JSON::Object::Ptr root = new Poco::JSON::Object();
                            root->set("updated", id);
                            std::ostream &ostr = response.send();
                            Poco::JSON::Stringifier::stringify(root, ostr);

                            return;
                        }
                        else
                        {
                            notFoundError(response, request.getURI(), "Order not found");
                            return;
                        }
                    }
                    else
                    {
                        badRequestError(response, request.getURI());
                        return;
                    }
                }
                
            }
            
        }
        catch (...)//(const Poco::Exception& e)
        {
            //std::cout<<e.displayText()<<std::endl;
        }
        notFoundError(response, request.getURI(), "Request not found");
    }

private:
    std::string _format;
};
#endif