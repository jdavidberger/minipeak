#pragma once

#include "string"
#include "stdlib.h"
#include "stdint.h"

namespace minipeak {
    namespace gpu {
        struct DeviceInfo_t {
            std::string deviceName, driverVersion;
            uint64_t numCUs;
            uint64_t computeWgsPerCU;
            uint maxWGSize;
            uint64_t maxAllocSize;
            uint64_t maxGlobalSize;
            uint64_t deviceType;
        };
    }
}