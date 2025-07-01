#pragma once

#include "scalar_storage.hh"
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

    /// Snapshot
    void takeSnapshot(ScalarStorage &scalar_storage);
    void loadSnapshot(ScalarStorage &scalar_storage);
    void saveLastSnapshotID();
    void loadLastSnapshotID();

  private:
    u64 m_last_snapshot_id;
    u64 m_increase_id;
    std::fstream m_wal_log_file;
};

} // namespace vdb
