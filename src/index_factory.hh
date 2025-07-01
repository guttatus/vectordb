#pragma once

#include "faiss_index.hh"
#include "filter_index.hh"
#include <format>
#include <map>
#include <rapidjson/document.h>

namespace vdb
{
class IndexFactory
{
  public:
    enum class IndexType
    {
        FLAT,
        HNSW,
        FILTER,
        UNKNOWN = -1
    };

    enum class MetricType
    {
        L2, // L2 距离
        IP  // 内积
    };

    ~IndexFactory();
    void init(IndexType type, i32 dim, MetricType metric = MetricType::L2);

    /// observe
    void *getIndex(IndexType type) const;
    FaissIndex *getFaissIndex(IndexType type) const;
    FilterIndex *getFilterIndex() const;

    /// snapshot
    void saveIndex(const std::string folder_path, ScalarStorage &scalar_storage);
    void loadIndex(const std::string folder_path, ScalarStorage &scalar_storage);

  private:
    std::map<IndexType, void *> m_index_map;
};

IndexFactory *getGlobalIndexFactory();
IndexFactory::IndexType getIndexTypeFromJson(const rapidjson::Document &json_data);

} // namespace vdb

using namespace vdb;
template <> struct std::formatter<IndexFactory::IndexType>
{
    // 解析格式说明符（支持 's' 和 'd'）
    constexpr auto parse(std::format_parse_context &ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && (*it == 's' || *it == 'd'))
        {
            specifier = *it++;
        }
        if (it != ctx.end() && *it != '}')
        {
            throw std::format_error("无效格式说明符");
        }
        return it;
    }

    // 格式化函数
    auto format(IndexFactory::IndexType index, std::format_context &ctx) const
    {
        if (specifier == 'd')
        {
            return std::format_to(ctx.out(), "{}", static_cast<i32>(index));
        }

        // 默认字符串输出
        std::string_view str;
        switch (index)
        {
        case IndexFactory::IndexType::FLAT:
            str = "FLAT";
            break;
        case IndexFactory::IndexType::HNSW:
            str = "HNSW";
            break;
        case IndexFactory::IndexType::FILTER:
            str = "FILTER";
            break;
        default:
            str = "UNKNOWN";
        }
        return std::format_to(ctx.out(), "{}", str);
    }

  private:
    char specifier = 's'; // 默认格式：字符串
};
