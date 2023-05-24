#include "AppGlobal.h"
#include <memory>

#include "cinder/CinderAssert.h"
#include "cinder/app/Platform.h"

namespace juniper {

AppGlobal* global()
{
	static AppGlobal global;
	return &global;
}


const fs::path& getRepoRootPath( const fs::path &rootFile )
{
	static fs::path	sRepoRootPath;


	if( sRepoRootPath.empty() ) {
		const size_t maxDirectoryTraversals = 30;

		auto appPath = cinder::app::Platform::get()->getDefaultExecutablePath();

		size_t parentCount = 0;
		for( auto currentPath = appPath; currentPath.has_parent_path(); currentPath = currentPath.parent_path() ) {
			if( ++parentCount > maxDirectoryTraversals )
				break;

			if( ! rootFile.empty() ) {
				const fs::path p = currentPath / rootFile;
				if( fs::exists( p ) ) {
					sRepoRootPath = currentPath;
					break;
				}
			}

			const fs::path currentGitPath = currentPath / ".git";
			if( fs::exists( currentGitPath ) ) {
				sRepoRootPath = currentPath;
				break;
			}
		}

		CI_ASSERT( ! sRepoRootPath.empty() );
	}

	return sRepoRootPath;
}


} // namespace juniper
