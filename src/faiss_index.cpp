#include "faiss_index.hh"
#include "logger.hh"
#include <faiss/Index.h>
#include <faiss/IndexIDMap.h>
#include <faiss/impl/IDSelector.h>
#include <faiss/index_io.h>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace vdb
{

bool RoaringBitmapIDSelector::is_member(i64 id) const
{
    if (m_bitmap == nullptr)
    {
        return false;
    }
    return roaring_bitmap_contains(m_bitmap, static_cast<u32>(id));
}

FaissIndex::FaissIndex(faiss::Index *index) : m_index(index)
{
}

FaissIndex::~FaissIndex()
{
    delete m_index;
}

void FaissIndex::insert_vectors(const std::vector<f32> &data, u64 label)
{
    i64 id = static_cast<i64>(label);
    m_index->add_with_ids(1, data.data(), &id);
}

void FaissIndex::remove_vectors(const std::vector<i64> &ids)
{
    faiss::IndexIDMap *id_map = dynamic_cast<faiss::IndexIDMap *>(m_index);
    if (id_map)
    {
        faiss::IDSelectorBatch selector(ids.size(), ids.data());
        id_map->remove_ids(selector);
    }
    else
    {
        throw std::runtime_error("Underlying Faiss index is not an IndexIDMap");
    }
}

std::pair<std::vector<i64>, std::vector<f32>> FaissIndex::search_vectors(const std::vector<f32> &query, i32 k,
                                                                         const roaring_bitmap_t *bitmap)
{
    i32 dim = m_index->d;
    i32 query_num = query.size() / dim;
    std::vector<i64> labels(query_num * k);
    std::vector<f32> distances(query_num * k);

    faiss::SearchParameters search_params;
    RoaringBitmapIDSelector selector(bitmap);
    if (bitmap != nullptr)
    {
        search_params.sel = &selector;
    }

    m_index->search(query_num, query.data(), k, distances.data(), labels.data(), &search_params);

    GlobalLogger->debug("<FaissIndex> Retrieved values:");

    for (size_t i = 0; i < labels.size(); ++i)
    {
        if (labels[i] != -1)
        {
            GlobalLogger->debug("<FaissIndex> ID: {}, Distance: {}", labels[i], distances[i]);
        }
        else
        {
            GlobalLogger->debug("<FaissIndex> No specific value found");
        }
    }

    return {labels, distances};
}

void FaissIndex::saveIndex(const std::string &file_path)
{
    faiss::write_index(m_index, file_path.c_str());
}

void FaissIndex::loadIndex(const std::string &file_path)
{
    std::ifstream file(file_path);
    if (file.good()) // check if a file exists
    {
        file.close();
        if (m_index != nullptr)
        {
            delete m_index;
        }
        m_index = faiss::read_index(file_path.c_str());
    }
    else
    {
        GlobalLogger->warn("<FaissIndex> File not found: {}, Skipping loading index.", file_path);
    }
}

} // namespace vdb
