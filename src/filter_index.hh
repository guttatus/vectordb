#pragma once

#include "scalar_storage.hh"
#include "types.hh"
#include <map>
#include <optional>
#include <roaring/roaring.h>
#include <string>

namespace vdb
{
class FilterIndex
{
  public:
    enum class Operation
    {
        EQUAL,
        NOT_EQUAL,
    };

    using filed_t = std::string;

    FilterIndex();

    /// Modify
    void addIntFieldFilter(const std::string &fieldname, i64 value, u64 id);
    void updateIntFieldFilter(const std::string &fieldname, i64 new_value, u64 id,
                              std::optional<i64> old_value = std::nullopt);
    /// Observe
    void getIntFieldFilterBitmap(const std::string &fieldname, Operation op, i64 value, roaring_bitmap_t *bitmap);

    /// Snapshot
    std::string serializeIntFiledFilter();
    void deserializeIntFiledFilter(const std::string &serialized_data);
    void saveIndex(ScalarStorage &scalar_storage, const std::string &key);
    void loadIndex(ScalarStorage &scalar_storage, const std::string &key);

  private:
    std::map<filed_t, std::map<i64, roaring_bitmap_t *>> m_int_field_filter;
};
} // namespace vdb
