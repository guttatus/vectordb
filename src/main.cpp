#include "http_server.hh"
#include "index_factory.hh"
#include "logger.hh"
#include <spdlog/common.h>

int main(int argc, char **argv) {
  using namespace vdb;
  init_global_logger();
  set_log_level(spdlog::level::debug);
  GlobalLogger->info("Global logger initialized");

  // 初始化全局IndexFactor实例
  int dim = 1;
  IndexFactory *globalIndexFactory = getGlobalIndexFactory();
  globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
  globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim);
  GlobalLogger->info("Global IndexFactory initialized");

  HttpServer server("localhost", 8080);
  GlobalLogger->info("HttpServer Start");
  server.start();
  return 0;
}
