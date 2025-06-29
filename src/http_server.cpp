#include "http_server.hh"
#include "constants.hh"
#include "faiss_index.hh"
#include "index_factory.hh"
#include "logger.hh"
#include <cstddef>
#include <format>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <utility>
#include <vector>

namespace vdb {

HttpServer::HttpServer(const std::string &host, int port)
    : m_host(host), m_port(port) {
  m_server.Post("/search",
                [this](const httplib::Request &req, httplib::Response &res) {
                  searchHandler(req, res);
                });

  m_server.Post("/insert",
                [this](const httplib::Request &req, httplib::Response &res) {
                  insertHandler(req, res);
                });
}

void HttpServer::start() { m_server.listen(m_host.c_str(), m_port); }

void HttpServer::searchHandler(const httplib::Request &req,
                               httplib::Response &res) {
  GlobalLogger->debug("Received search request");
  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());
  GlobalLogger->info("Search request parameters: {}", req.body);

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid json request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  if (!isRequestValid(json_request, CheckType::SEARCH)) {
    GlobalLogger->error("Missing vectors or k parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Missing vectors or k parameter in the request");
    return;
  }

  std::vector<f32> query;
  for (const auto &q : json_request[REQUEST_VECTORS].GetArray()) {
    query.push_back(q.GetFloat());
  }
  int k = json_request[REQUEST_K].GetInt();

  GlobalLogger->debug("Query parameters: k = {}", k);

  IndexFactory::IndexType index_type = getIndexTypeFromRequset(json_request);
  if (index_type == IndexFactory::IndexType::UNKNOWN) {
    GlobalLogger->error("Invalid index type parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Invalid index type parameter in the request");
    return;
  }

  FaissIndex *index = getGlobalIndexFactory()->getIndex(index_type);

  if (index == nullptr) {
    std::string error_msg =
        std::format("Index type {} is not supported", index_type);

    GlobalLogger->error(error_msg);
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, error_msg);
    return;
  }

  auto results = index->search_vectors(query, k);

  // 将结果转换为JSON格式
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  bool valid_results = false;
  rapidjson::Value vectors(rapidjson::kArrayType);
  rapidjson::Value distances(rapidjson::kArrayType);
  for (size_t i = 0; i < results.first.size(); ++i) {
    if (results.first[i] != -1) {
      valid_results = true;
      vectors.PushBack(results.first[i], allocator);
      distances.PushBack(results.second[i], allocator);
    }
  }

  if (valid_results) {
    json_response.AddMember(RESPONSE_VECTORS, vectors, allocator);
    json_response.AddMember(RESPONSE_DISTANCES, distances, allocator);
  }

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          allocator);
  setJsonResponse(json_response, res);
}

void HttpServer::insertHandler(const httplib::Request &req,
                               httplib::Response &res) {

  GlobalLogger->debug("Received insert request");
  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());
  GlobalLogger->info("Insert request parameters: {}", req.body);

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid json request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  if (!isRequestValid(json_request, CheckType::INSERT)) {
    GlobalLogger->error("Missing vectors or id parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Missing vectors or id parameter in the request");
    return;
  }

  std::vector<f32> data;
  for (const auto &d : json_request[REQUEST_VECTORS].GetArray()) {
    data.push_back(d.GetFloat());
  }
  int label = json_request[REQUEST_ID].GetUint64();

  GlobalLogger->debug("Insert parameters: label = {}", label);

  IndexFactory::IndexType index_type = getIndexTypeFromRequset(json_request);
  if (index_type == IndexFactory::IndexType::UNKNOWN) {
    GlobalLogger->error("Invalid index type parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Invalid index type parameter in the request");
    return;
  }

  FaissIndex *index = getGlobalIndexFactory()->getIndex(index_type);

  if (index == nullptr) {
    std::string error_msg =
        std::format("Index type {} is not supported", index_type);

    GlobalLogger->error(error_msg);
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, error_msg);
    return;
  }

  index->insert_vectors(data, label);

  // 将结果转换为JSON格式
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          allocator);
  setJsonResponse(json_response, res);
}

void HttpServer::setJsonResponse(const rapidjson::Document &json_response,
                                 httplib::Response &res) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_response.Accept(writer);
  res.set_content(buffer.GetString(), RESPONSE_CONTENT_TYPE_JSON);
}

void HttpServer::setErrorJsonResponse(httplib::Response &res, int error_code,
                                      const std::string &error_msg) {
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();
  json_response.AddMember(RESPONSE_RETCODE, error_code, allocator);
  json_response.AddMember(RESPONSE_ERROR_MSG,
                          rapidjson::StringRef(error_msg.c_str()), allocator);
  setJsonResponse(json_response, res);
}

bool HttpServer::isRequestValid(const rapidjson::Document &json_request,
                                CheckType check_type) {
  switch (check_type) {
  case CheckType::SEARCH:
    return json_request.HasMember(REQUEST_VECTORS) &&
           json_request.HasMember(REQUEST_K) &&
           (!json_request.HasMember(REQUEST_INDEX_TYPE) ||
            json_request[REQUEST_INDEX_TYPE].IsString());
  case CheckType::INSERT:
    return json_request.HasMember(REQUEST_VECTORS) &&
           json_request.HasMember(REQUEST_ID) &&
           (!json_request.HasMember(REQUEST_INDEX_TYPE) ||
            json_request[REQUEST_INDEX_TYPE].IsString());
  default:
    return false;
  }
}

IndexFactory::IndexType
HttpServer::getIndexTypeFromRequset(const rapidjson::Document &json_request) {
  if (json_request.HasMember(REQUEST_INDEX_TYPE)) {
    std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
    if (index_type_str == "FLAT") {
      return IndexFactory::IndexType::FLAT;
    }
    if (index_type_str == "HNSW") {
      return IndexFactory::IndexType::HNSW;
    }
  }
  return IndexFactory::IndexType::UNKNOWN;
}
} // namespace vdb
