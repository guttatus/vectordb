#pragma once
#include "index_factory.hh"
#include "scalar_storage.hh"
#include <rapidjson/document.h>
#include <string>
#include <utility>
#include <vector>

namespace vdb
{
class VectorDB
{
  public:
    VectorDB(const std::string &db_path);

    void upsert(u64 id, const rapidjson::Document &data, IndexFactory::IndexType index_type);

    rapidjson::Document query(u64 id);

    std::pair<std::vector<i64>, std::vector<f32>> search(const rapidjson::Document &json_request);

  private:
    ScalarStorage m_scalar_storage;
};
} // namespace vdb
