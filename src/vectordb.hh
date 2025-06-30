#pragma once
#include "index_factory.hh"
#include "persistence.hh"
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
    VectorDB(const std::string &db_path, const std::string &wal_path);

    /// Modify
    void upsert(u64 id, const rapidjson::Document &data, IndexFactory::IndexType index_type);

    /// Observe
    rapidjson::Document query(u64 id);
    std::pair<std::vector<i64>, std::vector<f32>> search(const rapidjson::Document &json_request);

    /// WAL
    void writeWALLog(const std::string &operation_type, const rapidjson::Document &json_data);
    void reloadDataBase();

  private:
    ScalarStorage m_scalar_storage;
    Persistence m_persistence;
};
} // namespace vdb
