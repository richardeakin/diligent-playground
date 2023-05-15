#pragma once

namespace juniper {

namespace dg = Diligent;

} // namespace juniper

namespace ju = juniper;

// Define some Logging helpers, will be removed

#define JU_LOG_INFO( ... )  LOG_INFO_MESSAGE( __FUNCTION__, ##__VA_ARGS__ )
