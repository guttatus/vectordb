#include "vectordb.hh"
#include "constants.hh"
#include "index_factory.hh"
#include <stdexcept>
#include <vector>

namespace vdb {

VectorDB::VectorDB(const std::string &db_path) : m_scalar_storage(db_path) {}

void VectorDB::upsert(u64 id, const rapidjson::Document &data,
                      IndexFactory::IndexType index_type) {
  rapidjson::Document existing_data;
  try {
    existing_data = m_scalar_storage.get_scalar(id);
  } catch (const std::runtime_error &e) {
    // vector isn't exist, continue to insert
  }

  if (existing_data.IsObject()) {
    size_t existing_vecotrs_size = existing_data[REQUEST_VECTORS].Size();
    std::vector<f32> existing_vector(existing_vecotrs_size);
    for (rapidjson::SizeType i = 0; i < existing_vecotrs_size; ++i) {
      existing_vector[i] = existing_data[REQUEST_VECTORS][i].GetFloat();
    }

    FaissIndex *index = getGlobalIndexFactory()->getIndex(index_type);
    if (index) {
      index->remove_vectors({static_cast<i64>(id)});
    }

    size_t new_vectors_size = data[REQUEST_VECTORS].Size();
    std::vector<f32> new_vector(new_vectors_size);
    for (rapidjson::SizeType i = 0; i < new_vectors_size; ++i) {
      new_vector[i] = data[REQUEST_VECTORS][i].GetFloat();
    }

    if (index) {
      index->insert_vectors(new_vector, id);
    }

    m_scalar_storage.insert_scalar(id, data);
  }
}

rapidjson::Document VectorDB::query(u64 id) {
  return m_scalar_storage.get_scalar(id);
}
} // namespace vdb
