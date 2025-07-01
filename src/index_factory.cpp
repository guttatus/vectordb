#include "index_factory.hh"
#include "constants.hh"
#include "filter_index.hh"
#include "logger.hh"
#include <faiss/IndexFlat.h>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIDMap.h>
#include <format>

namespace vdb
{

namespace
{
IndexFactory globalIndexFactory;
}

IndexFactory *getGlobalIndexFactory()
{
    return &globalIndexFactory;
}

IndexFactory::~IndexFactory()
{
    for (auto [index_type, index_ptr] : m_index_map)
    {
        switch (index_type)
        {
        case IndexType::FLAT:
        case IndexType::HNSW: {
            delete static_cast<FaissIndex *>(index_ptr);
            break;
        }
        case IndexType::FILTER: {
            delete static_cast<FilterIndex *>(index_ptr);
            break;
        }
        default:
            break;
        }
    }
}

void IndexFactory::init(IndexType type, i32 dim, MetricType metric)
{
    faiss::MetricType faiss_metric = (metric == MetricType::L2) ? faiss::METRIC_L2 : faiss::METRIC_INNER_PRODUCT;

    void *index = getIndex(type);
    if (index != nullptr)
    {
        std::string error_msg = std::format("Index type {} has already been iniyialized", type);
        GlobalLogger->error("<IndexFactory>" + error_msg);
        return;
    }

    switch (type)
    {
    case IndexType::FLAT: {
        m_index_map[type] = new FaissIndex(new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
        break;
    }
    case IndexType::HNSW: {
        m_index_map[type] = new FaissIndex(new faiss::IndexIDMap(new faiss::IndexHNSWFlat(dim, 32, faiss_metric)));
        break;
    }
    case IndexType::FILTER: {
        m_index_map[type] = new FilterIndex();
        break;
    }
    default:
        break;
    }
}

void *IndexFactory::getIndex(IndexType type) const
{
    auto it = m_index_map.find(type);
    if (it != m_index_map.end())
    {
        return it->second;
    }
    return nullptr;
}

FaissIndex *IndexFactory::getFaissIndex(IndexType type) const
{
    if (type != IndexType::HNSW && type != IndexType::FLAT)
    {
        return nullptr;
    }
    auto it = m_index_map.find(type);
    if (it != m_index_map.end())
    {
        return static_cast<FaissIndex *>(it->second);
    }
    return nullptr;
}

FilterIndex *IndexFactory::getFilterIndex() const
{
    auto it = m_index_map.find(IndexType::FILTER);
    if (it != m_index_map.end())
    {
        return static_cast<FilterIndex *>(it->second);
    }
    return nullptr;
}

void IndexFactory::saveIndex(const std::string folder_path, ScalarStorage &scalar_storage)
{
    for (const auto &[index_type, index_ptr] : m_index_map)
    {
        std::string file_path = std::format("{}.{}.index", folder_path, index_type);
        if (index_type == IndexType::FLAT || index_type == IndexType::HNSW)
        {
            static_cast<FaissIndex *>(index_ptr)->saveIndex(file_path);
        }
        else if (index_type == IndexType::FILTER)
        {
            static_cast<FilterIndex *>(index_ptr)->saveIndex(scalar_storage, file_path);
        }
    }
}

void IndexFactory::loadIndex(const std::string folder_path, ScalarStorage &scalar_storage)
{
    for (const auto &[index_type, index_ptr] : m_index_map)
    {
        std::string file_path = std::format("{}.{}.index", folder_path, index_type);
        if (index_type == IndexType::FLAT || index_type == IndexType::HNSW)
        {
            static_cast<FaissIndex *>(index_ptr)->loadIndex(file_path);
        }
        else if (index_type == IndexType::FILTER)
        {
            static_cast<FilterIndex *>(index_ptr)->loadIndex(scalar_storage, file_path);
        }
    }
}

IndexFactory::IndexType getIndexTypeFromJson(const rapidjson::Document &json_data)
{
    if (json_data.HasMember(REQUEST_INDEX_TYPE))
    {
        std::string index_type_str = json_data[REQUEST_INDEX_TYPE].GetString();
        if (index_type_str == "FLAT")
        {
            return IndexFactory::IndexType::FLAT;
        }
        if (index_type_str == "HNSW")
        {
            return IndexFactory::IndexType::HNSW;
        }
    }
    return IndexFactory::IndexType::UNKNOWN;
}

} // namespace vdb
