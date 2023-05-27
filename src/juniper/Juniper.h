#pragma once

namespace juniper {

namespace dg = Diligent;

} // namespace juniper

namespace ju = juniper;

// Define some Logging helpers, TODO: remove and use ci::log macros

#define JU_LOG_INFO( ... )		LOG_INFO_MESSAGE( __FUNCTION__, "| ", ##__VA_ARGS__ )
#define JU_LOG_WARNING( ... )	LOG_WARNING_MESSAGE( __FUNCTION__, "| ", ##__VA_ARGS__ )
#define JU_LOG_ERROR( ... )		LOG_ERROR_MESSAGE( __FUNCTION__, "| ", ##__VA_ARGS__ )
