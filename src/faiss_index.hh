#pragma once

#include "types.hh"
#include <faiss/Index.h>
#include <utility>
#include <vector>

namespace vdb {

class FaissIndex {
public:
  ~FaissIndex();
  FaissIndex(faiss::Index *index);
  void insert_vectors(const std::vector<f32> &data, u64 label);
  std::pair<std::vector<i64>, std::vector<f32>>
  search_vectors(const std::vector<f32> &query, i32 k);

private:
  faiss::Index *m_index;
};

} // namespace vdb
