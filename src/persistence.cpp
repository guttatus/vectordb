#include "persistence.hh"
#include "index_factory.hh"
#include "logger.hh"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <cstring>
#include <fstream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace vdb
{

Persistence::Persistence() : m_increase_id(0), m_last_snapshot_id(0)
{
}

Persistence::~Persistence()
{
    if (m_wal_log_file.is_open())
    {
        m_wal_log_file.close();
    }
}

void Persistence::init(const std::string &local_path)
{
    m_wal_log_file.open(local_path, std::ios::in | std::ios::out | std::ios::app);
    if (!m_wal_log_file.is_open())
    {
        GlobalLogger->error("<Persistence> Failed to open WAL log {}, Reason: {}", local_path, std::strerror(errno));
        throw std::runtime_error("<Persistence> Failed to open WAL at path: " + local_path);
    }
    loadLastSnapshotID();
}

u64 Persistence::increaseID()
{
    return ++m_increase_id;
}

u64 Persistence::getID() const
{
    return m_increase_id;
}

void Persistence::writeWALLog(const std::string &operation_type, const rapidjson::Document &json_data,
                              const std::string &version)
{
    u64 log_id = increaseID();

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_data.Accept(writer);

    m_wal_log_file << log_id << "|" << version << "|" << operation_type << "|" << buffer.GetString() << std::endl;

    if (m_wal_log_file.fail())
    {
        GlobalLogger->error("<Persistence> A error occurred while writing the WAL log entry. Reason: {}",
                            std::strerror(errno));
    }
    else
    {
        m_wal_log_file.flush();
    }
}
void Persistence::readNextWALLog(std::string *operation_type, rapidjson::Document *json_data)
{
    GlobalLogger->debug("<Persistence> Reading next WAL log entry");
    std::string line;
    while (std::getline(m_wal_log_file, line))
    {
        std::istringstream iss(line);
        std::string log_id_str, version, json_data_str;
        std::getline(iss, log_id_str, '|');
        std::getline(iss, version, '|');
        std::getline(iss, *operation_type, '|');
        std::getline(iss, json_data_str, '|');

        u64 log_id = std::stoull(log_id_str);
        if (log_id > m_increase_id)
        {
            m_increase_id = log_id;
        }

        if (log_id > m_last_snapshot_id)
        {
            json_data->Parse(json_data_str.c_str());
            GlobalLogger->debug("<Persistence> Read WAL log entry: log_id={}, operation_type={}, json_data_str={}",
                                log_id, *operation_type, json_data_str);
            return;
        }
        else
        {
            GlobalLogger->debug("<Persistence> Skip Read WAL log entry: log_id={}", log_id);
        }
    }
    operation_type->clear();
    m_wal_log_file.clear();
    GlobalLogger->debug("<Persistence> No more WAL log entries to read");
}

void Persistence::takeSnapshot(ScalarStorage &scalar_storage)
{
    GlobalLogger->debug("<Persistence> Taking Snapshot");
    m_last_snapshot_id = m_increase_id;
    std::string snapshot_folder_path = "vdb.snapshot";
    IndexFactory *index_factory = getGlobalIndexFactory();
    index_factory->saveIndex(snapshot_folder_path, scalar_storage);
    saveLastSnapshotID();
}

void Persistence::loadSnapshot(ScalarStorage &scalar_storage)
{
    GlobalLogger->debug("<Persistence> Loading Snapshot");
    std::string snapshot_folder_path = "vdb.snapshot";
    IndexFactory *index_factory = getGlobalIndexFactory();
    index_factory->loadIndex(snapshot_folder_path, scalar_storage);
}

void Persistence::saveLastSnapshotID()
{
    std::ofstream file("vdb.snapshot.maxlogid");
    if (file.is_open())
    {
        file << m_last_snapshot_id;
        file.close();
    }
    else
    {
        GlobalLogger->error("<Persistence> Failed to open file vdb.snapshot.maxlogid for writing");
    }
    GlobalLogger->debug("<Persistence> Save snapshot Max log ID {}", m_last_snapshot_id);
}

void Persistence::loadLastSnapshotID()
{
    std::ifstream file("vdb.snapshot.maxlogid");
    if (file.is_open())
    {
        file >> m_last_snapshot_id;
        file.close();
    }
    else
    {
        GlobalLogger->error("<Persistence> Failed to open file vdb.snapshot.maxlogid for reading");
    }
    GlobalLogger->debug("<Persistence> Load snapshot Max log ID {}", m_last_snapshot_id);
}

} // namespace vdb
