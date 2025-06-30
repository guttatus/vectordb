#pragma once

#include "types.hh"
#include <fstream>
#include <rapidjson/document.h>
#include <string>

namespace vdb
{

class Persistence
{
  public:
    Persistence();
    ~Persistence();
    void init(const std::string &local_path);
    u64 increaseID();
    u64 getID() const;
    void writeWALLog(const std::string &operation_type, const rapidjson::Document &json_data,
                     const std::string &version);
    void readNextWALLog(std::string *operation_type, rapidjson::Document *json_data);

  private:
    u64 m_increase_id;
    std::fstream m_wal_log_file;
};

} // namespace vdb
