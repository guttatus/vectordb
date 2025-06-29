#pragma once

#include "index_factory.hh"
#include "vectordb.hh"
#include <httplib.h>
#include <rapidjson/document.h>
#include <string>

namespace vdb {
class HttpServer {
public:
  enum class CheckType { SEARCH, INSERT, UPSERT, QQUERY };

  HttpServer(const std::string &host, i32 port, VectorDB *vdb);
  void start();

private:
  void searchHandler(const httplib::Request &req, httplib::Response &res);
  void insertHandler(const httplib::Request &req, httplib::Response &res);
  void upsertHandler(const httplib::Request &req, httplib::Response &res);
  void queryHandler(const httplib::Request &req, httplib::Response &res);
  void setJsonResponse(const rapidjson::Document &json_response,
                       httplib::Response &res);
  void setErrorJsonResponse(httplib::Response &res, i32 error_code,
                            const std::string &error_msg);
  bool isRequestValid(const rapidjson::Document &json_request,
                      CheckType check_type);

  IndexFactory::IndexType
  getIndexTypeFromRequset(const rapidjson::Document &json_request);

  httplib::Server m_server;
  std::string m_host;
  i32 m_port;
  VectorDB *m_vector_db;
};
} // namespace vdb
