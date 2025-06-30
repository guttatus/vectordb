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
    void insert_scalar(u64 id, const rapidjson::Document &data);
    rapidjson::Document get_scalar(u64 id);

  private:
    rocksdb::DB *m_db;
};

} // namespace vdb
