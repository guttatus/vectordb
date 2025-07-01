#include "filter_index.hh"
#include "logger.hh"
#include <optional>
#include <sstream>

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

/// Snap
std::string FilterIndex::serializeIntFiledFilter()
{
    std::ostringstream oss;
    for (const auto &field_entry : m_int_field_filter)
    {
        const std::string &field_name = field_entry.first;
        const auto &value_map = field_entry.second;
        for (const auto &value_entry : value_map)
        {
            i64 value = value_entry.first;
            const roaring_bitmap_t *bitmap = value_entry.second;

            // serializa bitmap
            u32 size = roaring_bitmap_portable_size_in_bytes(bitmap);
            char *serializa_bitmap = new char[size];
            roaring_bitmap_portable_serialize(bitmap, serializa_bitmap);
            oss << field_name << "|" << value << "|";
            oss.write(serializa_bitmap, size);
            oss << std::endl;
            delete[] serializa_bitmap;
        }
    }
    return oss.str();
}

void FilterIndex::deserializeIntFiledFilter(const std::string &serialized_data)
{
    std::istringstream iss(serialized_data);

    std::string line;
    while (std::getline(iss, line))
    {
        std::istringstream line_iss(line);

        std::string field_name;
        std::getline(line_iss, field_name, '|');

        std::string value_str;
        std::getline(line_iss, value_str, '|');
        long value = std::stol(value_str);

        std::string serialized_bitmap(std::istreambuf_iterator<char>(line_iss), {});

        roaring_bitmap_t *bitmap = roaring_bitmap_portable_deserialize(serialized_bitmap.data());

        m_int_field_filter[field_name][value] = bitmap;
    }
}

void FilterIndex::saveIndex(ScalarStorage &scalar_storage, const std::string &key)
{
    std::string serialized_data = serializeIntFiledFilter();
    scalar_storage.put(key, serialized_data);
}

void FilterIndex::loadIndex(ScalarStorage &scalar_storage, const std::string &key)
{
    std::string serialized_data = scalar_storage.get(key);
    deserializeIntFiledFilter(serialized_data);
}

} // namespace vdb
