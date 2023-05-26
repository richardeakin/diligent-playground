#include "LivePP.h"

//#include "../../Primitives/interface/Errors.hpp"

#include <windows.h>
#include "LPP_API_x64_CPP.h"

#include "cinder/Log.h"

using namespace std;
namespace fs = std::filesystem;

namespace juniper {

wstring toWideString( const string &utf8String )
{
	int wideSize = ::MultiByteToWideChar( CP_UTF8, 0, utf8String.c_str(), -1, NULL, 0 );
	if( wideSize == ERROR_NO_UNICODE_TRANSLATION ) {
		throw std::runtime_error( "Invalid UTF-8 sequence." );
	}
	else if( wideSize == 0 ) {
		throw std::runtime_error( "Error in UTF-8 to UTF-16 conversion." );
	}

	std::vector<wchar_t> resultString( wideSize );
	int convResult = ::MultiByteToWideChar( CP_UTF8, 0, utf8String.c_str(), -1, &resultString[0], wideSize );
	if( convResult != wideSize ) {
		throw std::runtime_error( "Error in UTF-8 to UTF-16 conversion." );
	}

	return std::wstring( &resultString[0] );
}

// TODO: should probably use the synchronized agent as explained here https://liveplusplus.tech/docs/documentation.html#creating_synchronized_agent
bool initLivePP( const fs::path &lppPath, const fs::path &settingsFile )
{
	auto exePath = lpp::LppGetCurrentModulePathANSI();
	//LOG_INFO_MESSAGE( string( "current module path: " ) + exePath );
	CI_LOG_I( "current module path: " << exePath );

	auto wp = toWideString( lppPath.string() );
	auto prefsFile = fs::path( exePath ).parent_path() / lppPath / settingsFile;

	CI_LOG_I( "prefsFile: " << prefsFile );

	lpp::LppDefaultAgent agent;
	if( fs::exists( prefsFile ) ) {
		CI_LOG_I( "\t- found settings file" );
		auto wprefs = toWideString( prefsFile.string() );
		agent = lpp::LppCreateDefaultAgentWithPreferencesFromFile( wp.c_str(), wprefs.c_str() );
	}
	else {
		CI_LOG_E( "could not find settings file" );
		agent = lpp::LppCreateDefaultAgent( wp.c_str() );
	}

	agent.EnableModule( lpp::LppGetCurrentModulePath(), lpp::LPP_MODULES_OPTION_ALL_IMPORT_MODULES );

    return true; // TODO: use return from Live++
}

// TODO: expose and test
void destroyLivePP()
{
    // destroy the Live++ agent
    //lpp::LppDestroyDefaultAgent(&lppAgent);	
}

} // namespace juniper