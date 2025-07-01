#pragma once

#include "types.hh"
#include <rapidjson/document.h>
#include <rocksdb/db.h>
#include <string>

namespace vdb
{
class ScalarStorage
{
  public:
    ScalarStorage(const std::string &db_path);
    ~ScalarStorage();

    /// modify
    void insert_scalar(u64 id, const rapidjson::Document &data);
    void put(const std::string &key, const std::string &value);

    /// observe
    rapidjson::Document get_scalar(u64 id);
    std::string get(const std::string &key);

  private:
    rocksdb::DB *m_db;
};

} // namespace vdb
