#include "AppGlobal.h"
#include <memory>

namespace app {

AppGlobal* global()
{
	static AppGlobal global;
	return &global;
}

} // namespace app