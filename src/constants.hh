#pragma once

#include "types.hh"
namespace vdb {

#define LOGGER_NAME "GlobalLogger"
#define RESPONSE_VECTORS "vectors"
#define RESPONSE_DISTANCES "distances"
#define REQUEST_VECTORS "vectors"
#define REQUEST_K "k"
#define REQUEST_ID "id"
#define REQUEST_INDEX_TYPE "indexType"

#define RESPONSE_RETCODE "retCode"

#define RESPONSE_ERROR_MSG "errorMsg"

#define RESPONSE_CONTENT_TYPE_JSON "application/json"

const i32 RESPONSE_RETCODE_SUCCESS = 0;
const i32 RESPONSE_RETCODE_ERROR = -1;

} // namespace vdb
