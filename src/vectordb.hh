#pragma once
#include "index_factory.hh"
#include "scalarstorage.hh"
#include <rapidjson/document.h>
#include <string>

namespace vdb {
class VectorDB {
public:
  VectorDB(const std::string &db_path);
  void upsert(u64 id, const rapidjson::Document &data,
              IndexFactory::IndexType index_type);
  rapidjson::Document query(u64 id);

private:
  ScalarStorage m_scalar_storage;
};
} // namespace vdb
