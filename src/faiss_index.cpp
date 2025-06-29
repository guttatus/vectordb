#include "faiss_index.hh"
#include "logger.hh"
#include <vector>

namespace vdb {
FaissIndex::FaissIndex(faiss::Index *index) : m_index(index) {}

FaissIndex::~FaissIndex() { delete m_index; }

void FaissIndex::insert_vectors(const std::vector<f32> &data, u64 label) {
  i64 id = static_cast<i64>(label);
  m_index->add_with_ids(1, data.data(), &id);
}

std::pair<std::vector<i64>, std::vector<f32>>
FaissIndex::search_vectors(const std::vector<f32> &query, i32 k) {
  int dim = m_index->d;
  int query_num = query.size() / dim;
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
