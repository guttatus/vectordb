#include "http_server.hh"
#include "index_factory.hh"
#include "logger.hh"
#include <spdlog/common.h>

int main(int argc, char **argv)
{
    using namespace vdb;
    init_global_logger();
    set_log_level(spdlog::level::debug);
    GlobalLogger->info("Global logger initialized");

    // 初始化全局IndexFactor实例
    int dim = 1;
    IndexFactory *globalIndexFactory = getGlobalIndexFactory();
    globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
    globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim);
    globalIndexFactory->init(IndexFactory::IndexType::FILTER, dim);
    GlobalLogger->info("Global IndexFactory initialized");

    std::string db_path = "VectorDB";
    std::string wal_path = "WALStorage";
    VectorDB vector_db(db_path, wal_path);
    vector_db.reloadDataBase();
    GlobalLogger->info("VectorDB initialized");

    HttpServer server("localhost", 8080, &vector_db);
    GlobalLogger->info("HttpServer Start");
    server.start();
    return 0;
}
