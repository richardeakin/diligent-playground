#include "AppGlobal.h"
#include <memory>

namespace juniper {

AppGlobal* global()
{
	static AppGlobal global;
	return &global;
}

} // namespace juniper
