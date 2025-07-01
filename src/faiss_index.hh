#pragma once

#include "faiss/impl/IDSelector.h"
#include "types.hh"
#include <faiss/Index.h>
#include <roaring/roaring.h>
#include <utility>
#include <vector>

namespace vdb
{

struct RoaringBitmapIDSelector : faiss::IDSelector
{
    RoaringBitmapIDSelector(const roaring_bitmap_t *bitmap) : m_bitmap(bitmap)
    {
    }

    bool is_member(i64 id) const final;
    ~RoaringBitmapIDSelector() override
    {
    }

    const roaring_bitmap_t *m_bitmap;
};

class FaissIndex
{
  public:
    FaissIndex(faiss::Index *index);
    ~FaissIndex();

    /// modify
    void insert_vectors(const std::vector<f32> &data, u64 label);
    void remove_vectors(const std::vector<i64> &ids);

    /// observe
    std::pair<std::vector<i64>, std::vector<f32>> search_vectors(const std::vector<f32> &query, i32 k,
                                                                 const roaring_bitmap_t *bitmap = nullptr);

    /// Snapshot
    void saveIndex(const std::string &file_path);
    void loadIndex(const std::string &file_path);

  private:
    faiss::Index *m_index;
};

} // namespace vdb
