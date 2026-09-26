#include "bdk_status.h"

const char *bdk_status_str(bdk_status_t status)
{
    switch (status) {
    case BDK_OK:
        return "OK";
    case BDK_ERR:
        return "ERR";
    case BDK_ERR_NOT_IMPL:
        return "NOT_IMPL";
    case BDK_ERR_NULL:
        return "NULL";
    case BDK_ERR_RANGE:
        return "RANGE";
    case BDK_ERR_STATE:
        return "STATE";
    case BDK_ERR_BUSY:
        return "BUSY";
    case BDK_ERR_TIMEOUT:
        return "TIMEOUT";
    case BDK_ERR_NACK:
        return "NACK";
    case BDK_ERR_NODATA:
        return "NODATA";
    default:
        return "?";
    }
}
