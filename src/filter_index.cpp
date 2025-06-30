#include "filter_index.hh"
#include "logger.hh"
#include <optional>

namespace vdb
{

FilterIndex::FilterIndex()
{
}

void FilterIndex::addIntFieldFilter(const std::string &fieldname, i64 value, u64 id)
{
    roaring_bitmap_t *bitmap = roaring_bitmap_create();
    roaring_bitmap_add(bitmap, id);
    m_int_field_filter[fieldname][value] = bitmap;
    GlobalLogger->debug("Added int field filter: fieldname={}, value={}, id={}", fieldname, value, id);
}

void FilterIndex::updateIntFieldFilter(const std::string &fieldname, i64 new_value, u64 id,
                                       std::optional<i64> old_value)
{
    auto it = m_int_field_filter.find(fieldname);
    if (it != m_int_field_filter.end())
    {
        std::map<i64, roaring_bitmap_t *> &value_map = it->second;
        auto old_bitmap_it = (old_value.has_value()) ? value_map.find(old_value.value()) : value_map.end();

        if (old_bitmap_it != value_map.end())
        {
            roaring_bitmap_t *old_bitmap = old_bitmap_it->second;
            roaring_bitmap_remove(old_bitmap, id);
        }

        auto new_bitmap_it = value_map.find(new_value);
        if (new_bitmap_it == value_map.end())
        {
            roaring_bitmap_t *new_bitmap = roaring_bitmap_create();
            value_map[new_value] = new_bitmap;
            new_bitmap_it = value_map.find(new_value);
        }

        roaring_bitmap_t *new_bitmap = new_bitmap_it->second;
        roaring_bitmap_add(new_bitmap, id);
    }
    else
    {
        addIntFieldFilter(fieldname, new_value, id);
    }
}

void FilterIndex::getIntFieldFilterBitmap(const std::string &fieldname, Operation op, i64 value,
                                          roaring_bitmap_t *bitmap)
{
    auto it = m_int_field_filter.find(fieldname);
    if (it != m_int_field_filter.end())
    {
        auto &value_map = it->second;

        if (op == Operation::EQUAL)
        {
            auto bitmap_it = value_map.find(value);
            if (bitmap_it != value_map.end())
            {
                GlobalLogger->debug("Retrieved EQUAL bitmap for fieldname={}, value={}", fieldname, value);
                roaring_bitmap_or_inplace(bitmap, bitmap_it->second);
            }
        }
        else if (op == Operation::NOT_EQUAL)
        {
            for (const auto &entry : value_map)
            {
                if (entry.first != value)
                {
                    roaring_bitmap_or_inplace(bitmap, entry.second);
                }
            }
            GlobalLogger->debug("Retrieved NOT_EQUAL bitmap for fieldname={}, value={}", fieldname, value);
        }
    }
}

} // namespace vdb
