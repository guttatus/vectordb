#include "scalar_storage.hh"
#include "logger.hh"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rocksdb/options.h>
#include <rocksdb/status.h>
#include <stdexcept>
#include <string>

namespace vdb
{
ScalarStorage::ScalarStorage(const std::string &db_path)
{
    rocksdb::Options options;
    options.create_if_missing = true;
    rocksdb::Status status = rocksdb::DB::Open(options, db_path, &m_db);
    if (!status.ok())
    {
        throw std::runtime_error("<RocksDB> Failed to open RocksDB: " + status.ToString());
    }
}

ScalarStorage::~ScalarStorage()
{
    delete m_db;
}

void ScalarStorage::insert_scalar(u64 id, const rapidjson::Document &data)
{
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    std::string value = buffer.GetString();
    rocksdb::Status status = m_db->Put(rocksdb::WriteOptions(), std::to_string(id), value);
    if (!status.ok())
    {
        GlobalLogger->error("<RocksDB> Failed to insert scalar: {}", status.ToString());
    }
}

rapidjson::Document ScalarStorage::get_scalar(u64 id)
{
    std::string value;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), std::to_string(id), &value);

    if (!status.ok())
    {
        GlobalLogger->error("<RocksDB> No item with id {}", id);
        return rapidjson::Document();
    }

    rapidjson::Document data;
    data.Parse(value.c_str());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    data.Accept(writer);
    GlobalLogger->debug("<RocksDB> Data retrieved from ScalarStorage: {} : {}", buffer.GetString(), status.ToString());
    return data;
}

void ScalarStorage::put(const std::string &key, const std::string &value)
{
    rocksdb::Status status = m_db->Put(rocksdb::WriteOptions(), key, value);
    if (!status.ok())
    {
        GlobalLogger->error("<RocksDB> Failed to put key {} : {}", key, status.ToString());
    }
    return;
}

std::string ScalarStorage::get(const std::string &key)
{
    std::string value;
    rocksdb::Status status = m_db->Get(rocksdb::ReadOptions(), key, &value);
    if (!status.ok())
    {
        GlobalLogger->error("<RocksDB> Failed to get value for key {} : {}", key, status.ToString());
        return "";
    }
    return value;
}

} // namespace vdb
