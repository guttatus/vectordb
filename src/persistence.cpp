#include "persistence.hh"
#include "logger.hh"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include <cstring>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace vdb
{

Persistence::Persistence() : m_increase_id(0)
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
        GlobalLogger->error("Failed to open WAL log {}, Reason: {}", local_path, std::strerror(errno));
        throw std::runtime_error("Failed to open WAL at path: " + local_path);
    }
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
        GlobalLogger->error("A error occurred while writing the WAL log entry. Reason: {}", std::strerror(errno));
    }
    else
    {
        m_wal_log_file.flush();
    }
}
void Persistence::readNextWALLog(std::string *operation_type, rapidjson::Document *json_data)
{
    GlobalLogger->debug("Reading next WAL log entry");
    std::string line;
    if (std::getline(m_wal_log_file, line))
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

        json_data->Parse(json_data_str.c_str());
        GlobalLogger->debug("Read WAL log entry: log_id={}, operation_type={}, json_data_str={}", log_id,
                            *operation_type, json_data_str);
    }
    else
    {
        m_wal_log_file.clear();
        GlobalLogger->debug("No more WAL log entries to read");
    }
}

} // namespace vdb
