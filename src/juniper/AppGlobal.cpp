#include "AppGlobal.h"
#include <memory>

#if ! defined( CINDER_DISABLED )
#include "cinder/CinderAssert.h"
#include "cinder/app/Platform.h"
#include "cinder/Log.h"
#endif

namespace juniper {

AppGlobal* global()
{
	static AppGlobal global;
	return &global;
}

#if ! defined( CINDER_DISABLED )

void findRepoRootPath( const fs::path &rootFile )
{
	const size_t maxDirectoryTraversals = 30;

	auto appPath = cinder::app::Platform::get()->getDefaultExecutablePath();

	size_t parentCount = 0;
	for( auto currentPath = appPath; currentPath.has_parent_path(); currentPath = currentPath.parent_path() ) {
		if( ++parentCount > maxDirectoryTraversals )
			break;

		if( ! rootFile.empty() ) {
			const fs::path p = currentPath / rootFile;
			if( fs::exists( p ) ) {
				global()->repoRootPath = currentPath;
				break;
			}
		}

		const fs::path currentGitPath = currentPath / ".git";
		if( fs::exists( currentGitPath ) ) {
			global()->repoRootPath = currentPath;
			break;
		}
	}

	if( global()->repoRootPath.empty() ) {
		CI_LOG_E( "Failed to find repo root path (rootFile: " << rootFile << ")" );
	}
}

fs::path getRootAssetPath( const fs::path &relativePath )
{
	const auto &repoRoot = global()->repoRootPath;
	CI_ASSERT( ! repoRoot.empty() );

	auto result = repoRoot / "assets" / relativePath;
	if( fs::exists( result ) ) {
		return result;
	}

	// return empty path to signify failure
	return {};
}
#endif

} // namespace juniper
