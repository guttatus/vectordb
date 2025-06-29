#pragma once

#include "faiss_index.hh"
#include <format>
#include <map>

namespace vdb {
class IndexFactory {
public:
  enum class IndexType { FLAT, HNSW, UNKNOWN = -1 };

  enum class MetricType {
    L2, // L2 距离
    IP  // 内积
  };

  ~IndexFactory();
  void init(IndexType type, int dim, MetricType metric = MetricType::L2);
  FaissIndex *getIndex(IndexType type) const;

private:
  std::map<IndexType, FaissIndex *> m_index_map;
};

IndexFactory *getGlobalIndexFactory();

} // namespace vdb

using namespace vdb;
template <> struct std::formatter<IndexFactory::IndexType> {
  // 解析格式说明符（支持 's' 和 'd'）
  constexpr auto parse(std::format_parse_context &ctx) {
    auto it = ctx.begin();
    if (it != ctx.end() && (*it == 's' || *it == 'd')) {
      specifier = *it++;
    }
    if (it != ctx.end() && *it != '}') {
      throw std::format_error("无效格式说明符");
    }
    return it;
  }

  // 格式化函数
  auto format(IndexFactory::IndexType index, std::format_context &ctx) const {
    if (specifier == 'd') {
      return std::format_to(ctx.out(), "{}", static_cast<int>(index));
    }

    // 默认字符串输出
    std::string_view str;
    switch (index) {
    case IndexFactory::IndexType::FLAT:
      str = "FLAT";
      break;
    case IndexFactory::IndexType::HNSW:
      str = "HNSW";
      break;
    case IndexFactory::IndexType::UNKNOWN:
      str = "UNKNOWN";
      break;
    default:
      str = "INVALID";
    }
    return std::format_to(ctx.out(), "{}", str);
  }

private:
  char specifier = 's'; // 默认格式：字符串
};
