#include "faiss_index.hh"
#include "faiss/impl/IDSelector.h"
#include "logger.hh"
#include <faiss/IndexIDMap.h>
#include <stdexcept>
#include <vector>

namespace vdb {
FaissIndex::FaissIndex(faiss::Index *index) : m_index(index) {}

FaissIndex::~FaissIndex() { delete m_index; }

void FaissIndex::insert_vectors(const std::vector<f32> &data, u64 label) {
  i64 id = static_cast<i64>(label);
  m_index->add_with_ids(1, data.data(), &id);
}

void FaissIndex::remove_vectors(const std::vector<i64> &ids) {
  faiss::IndexIDMap *id_map = dynamic_cast<faiss::IndexIDMap *>(m_index);
  if (id_map) {
    faiss::IDSelectorBatch selector(ids.size(), ids.data());
    id_map->remove_ids(selector);
  } else {
    throw std::runtime_error("Underlying Faiss index is not an IndexIDMap");
  }
}

std::pair<std::vector<i64>, std::vector<f32>>
FaissIndex::search_vectors(const std::vector<f32> &query, i32 k) {
  i32 dim = m_index->d;
  i32 query_num = query.size() / dim;
  std::vector<i64> labels(query_num * k);
  std::vector<f32> distances(query_num * k);
  m_index->search(query_num, query.data(), k, distances.data(), labels.data());
  GlobalLogger->debug("Retrieved values:");

  for (size_t i = 0; i < labels.size(); ++i) {
    if (labels[i] != -1) {
      GlobalLogger->debug("ID: {}, Distance: {}", labels[i], distances[i]);
    } else {
      GlobalLogger->debug("No specific value found");
    }
  }

  return {labels, distances};
}

} // namespace vdb
