#pragma once

#include "index_factory.hh"
#include <httplib.h>
#include <rapidjson/document.h>
#include <string>

namespace vdb {
class HttpServer {
public:
  enum class CheckType { SEARCH, INSERT };

  HttpServer(const std::string &host, int port);
  void start();

private:
  void searchHandler(const httplib::Request &req, httplib::Response &res);
  void insertHandler(const httplib::Request &req, httplib::Response &res);
  void setJsonResponse(const rapidjson::Document &json_response,
                       httplib::Response &res);
  void setErrorJsonResponse(httplib::Response &res, int error_code,
                            const std::string &error_msg);
  bool isRequestValid(const rapidjson::Document &json_request,
                      CheckType check_type);

  IndexFactory::IndexType
  getIndexTypeFromRequset(const rapidjson::Document &json_request);

  httplib::Server m_server;
  std::string m_host;
  int m_port;
};
} // namespace vdb
