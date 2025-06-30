#include "vectordb.hh"
#include "constants.hh"
#include "faiss_index.hh"
#include "filter_index.hh"
#include "index_factory.hh"
#include "logger.hh"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include <cstdlib>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace vdb
{

VectorDB::VectorDB(const std::string &db_path, const std::string &wal_path) : m_scalar_storage(db_path), m_persistence()
{
    m_persistence.init(wal_path);
}

void VectorDB::upsert(u64 id, const rapidjson::Document &data, IndexFactory::IndexType index_type)
{
    rapidjson::Document existing_data;
    try
    {
        existing_data = m_scalar_storage.get_scalar(id);
    }
    catch (const std::runtime_error &e)
    {
        // vector isn't exist, continue to insert
    }

    if (existing_data.IsObject())
    {
        size_t existing_vecotrs_size = existing_data[REQUEST_VECTORS].Size();
        std::vector<f32> existing_vector(existing_vecotrs_size);
        for (rapidjson::SizeType i = 0; i < existing_vecotrs_size; ++i)
        {
            existing_vector[i] = existing_data[REQUEST_VECTORS][i].GetFloat();
        }

        FaissIndex *index = getGlobalIndexFactory()->getFaissIndex(index_type);
        if (index)
        {
            index->remove_vectors({static_cast<i64>(id)});
        }
    }

    size_t new_vectors_size = data[REQUEST_VECTORS].Size();
    std::vector<f32> new_vector(new_vectors_size);
    for (rapidjson::SizeType i = 0; i < new_vectors_size; ++i)
    {
        new_vector[i] = data[REQUEST_VECTORS][i].GetFloat();
    }

    GlobalLogger->debug("Add new id={} to index", id);
    FaissIndex *index = getGlobalIndexFactory()->getFaissIndex(index_type);
    if (index)
    {
        index->insert_vectors(new_vector, id);
    }

    GlobalLogger->debug("Try to add new filter");
    FilterIndex *filter_index = getGlobalIndexFactory()->getFilterIndex();
    if (filter_index)
    {
        for (auto it = data.MemberBegin(); it != data.MemberEnd(); ++it)
        {
            std::string field_name = it->name.GetString();
            if (it->value.IsInt() && field_name != REQUEST_ID)
            {
                i64 field_value = it->value.GetInt64();

                std::optional<i64> old_field_value = std::nullopt;
                if (existing_data.IsObject())
                {
                    old_field_value.emplace(existing_data[field_name.c_str()].GetInt64());
                }
                filter_index->updateIntFieldFilter(field_name, field_value, id, old_field_value);
            }
        }
    }

    m_scalar_storage.insert_scalar(id, data);
}

rapidjson::Document VectorDB::query(u64 id)
{
    return m_scalar_storage.get_scalar(id);
}

std::pair<std::vector<i64>, std::vector<f32>> VectorDB::search(const rapidjson::Document &json_request)
{
    std::vector<f32> query;
    for (const auto &q : json_request[REQUEST_VECTORS].GetArray())
    {
        query.push_back(q.GetFloat());
    }

    i32 k = json_request[REQUEST_K].GetInt();

    IndexFactory::IndexType index_type = IndexFactory::IndexType::UNKNOWN;
    if (json_request.HasMember(REQUEST_INDEX_TYPE) && json_request[REQUEST_INDEX_TYPE].IsString())
    {
        std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == INDEX_TYPE_FLAT)
        {
            index_type = IndexFactory::IndexType::FLAT;
        }
        else if (index_type_str == INDEX_TYPE_HNSW)
        {
            index_type = IndexFactory::IndexType::HNSW;
        }
    }

    roaring_bitmap_t *filter_bitmap = nullptr;
    if (json_request.HasMember(REQUEST_FILTER) && json_request[REQUEST_FILTER].IsObject())
    {
        const auto &filter = json_request[REQUEST_FILTER];
        // TODO: 检查下列字段合法性
        std::string field_name = filter[REQUEST_FILTER_NAME].GetString();
        std::string op_str = filter[REQUEST_FILTER_OP].GetString();
        i64 value = filter[REQUEST_FILTER_VALUE].GetInt64();

        FilterIndex::Operation op = (op_str == "=") ? FilterIndex::Operation::EQUAL : FilterIndex::Operation::NOT_EQUAL;

        FilterIndex *filter_index = getGlobalIndexFactory()->getFilterIndex();

        if (filter_index)
        {
            filter_bitmap = roaring_bitmap_create();
            filter_index->getIntFieldFilterBitmap(field_name, op, value, filter_bitmap);
        }
    }

    FaissIndex *index = getGlobalIndexFactory()->getFaissIndex(index_type);
    std::pair<std::vector<i64>, std::vector<f32>> results;
    if (index)
    {
        results = index->search_vectors(query, k, filter_bitmap);
    }

    if (filter_bitmap != nullptr)
    {
        delete filter_bitmap;
    }
    return results;
}

void VectorDB::writeWALLog(const std::string &operation_type, const rapidjson::Document &json_data)
{
    std::string version = "1.0";
    m_persistence.writeWALLog(operation_type, json_data, version);
}

void VectorDB::reloadDataBase()
{
    GlobalLogger->info("Entering VectorDB::reloadDataBase()");
    std::string operator_type;
    rapidjson::Document json_data;
    m_persistence.readNextWALLog(&operator_type, &json_data);

    while (!operator_type.empty())
    {
        GlobalLogger->debug("Operation Type: {}", operator_type);

        rapidjson::StringBuffer buffer;
        rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
        json_data.Accept(writer);
        GlobalLogger->info("Read Line: {}", buffer.GetString());

        if (operator_type == "upsert")
        {
            u64 id = json_data[REQUEST_ID].GetUint64();
            IndexFactory::IndexType index_type = getIndexTypeFromJson(json_data);
            upsert(id, json_data, index_type);
        }

        rapidjson::Document().Swap(json_data);
        operator_type.clear();
        m_persistence.readNextWALLog(&operator_type, &json_data);
    }
}

} // namespace vdb
