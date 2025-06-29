#include "index_factory.hh"
#include "faiss_index.hh"
#include "logger.hh"
#include <faiss/IndexFlat.h>
#include <faiss/IndexHNSW.h>
#include <faiss/IndexIDMap.h>
#include <format>

namespace vdb {

namespace {
IndexFactory globalIndexFactory;
}

IndexFactory *getGlobalIndexFactory() { return &globalIndexFactory; }

IndexFactory::~IndexFactory() {
  for (auto [index_type, index_ptr] : m_index_map) {
    delete index_ptr;
  }
}

void IndexFactory::init(IndexType type, i32 dim, MetricType metric) {
  faiss::MetricType faiss_metric = (metric == MetricType::L2)
                                       ? faiss::METRIC_L2
                                       : faiss::METRIC_INNER_PRODUCT;

  void *index = getIndex(type);
  if (index != nullptr) {
    std::string error_msg =
        std::format("Index type {} has already been iniyialized", type);
    GlobalLogger->error(error_msg);
    return;
  }

  switch (type) {
  case IndexType::FLAT: {
    m_index_map[type] = new FaissIndex(
        new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
    break;
  }
  case IndexType::HNSW: {
    m_index_map[type] = new FaissIndex(
        new faiss::IndexIDMap(new faiss::IndexHNSWFlat(dim, 32, faiss_metric)));
    break;
  }
  default:
    break;
  }
}
FaissIndex *IndexFactory::getIndex(IndexType type) const {
  auto it = m_index_map.find(type);
  if (it != m_index_map.end()) {
    return it->second;
  }
  return nullptr;
}

} // namespace vdb
