#include "Profiler.h"
#include "imgui.h"
#include <algorithm>

using namespace Diligent;
using namespace std;
namespace im = ImGui;

namespace ju {

Profiler::Profiler( IRenderDevice* device )
	: mDevice( device )
{
	if( ! mDevice->GetDeviceInfo().Features.TimestampQueries ) {
		LOG_ERROR( "Timestamp Queries not available" );
		return;
	}

	// TODO: pass in expected queries as optional param
	// TODO: make num queries grow to fit
	mQuerier = make_unique<DurationQueryHelper>( mDevice, 10 );
}

Profiler::~Profiler()
{
}

void Profiler::begin( IDeviceContext *context, const std::string &label )
{
	if( ! mQuerier ) {
		return;
	}

	mGpuDurations[label] = -1.0;
	mQuerier->Begin( context );
}

void Profiler::end( IDeviceContext *context, const std::string &label )
{
	if( ! mQuerier ) {
		return;
	}

	auto it = mGpuDurations.find( label );
	assert( it != mGpuDurations.end() );

	mQuerier->End( context, it->second );
}

//void Profiler::update( double elapsedTime )
//{
//	if( ! mQuerier ) {
//		return;
//	}
//}

void Profiler::updateUI( bool *open )
{

	if( ! im::Begin( "Profiling", open ) ) {
		im::End();
		return;
	}

	if( ! mQuerier ) {
		im::Text( "Timestamp Queries not supported on this device." );
		im::End();
		return;
	}

	// TODO: use table ui
	auto displayTimeFn = []( const pair<string,double> &t ) {
		im::Text( "%s", t.first.c_str() );
		im::NextColumn();
		im::Text( "%6.3f", (float)t.second );
		im::NextColumn();
	};

#if 0
	// TODO: switch to chrono for this
	static Timer  timer{ true };
	static double time = timer.getSeconds();
	const double  elapsed = timer.getSeconds() - time;
	time += elapsed;

	static size_t                      fpsIndex = 0;
	static std::array<float, 180>      fps;
	float currentFps = float( floor( 1.0 / elapsed ) );
	fps[fpsIndex++ % fps.size()] = currentFps;
	if( im::CollapsingHeader( ( "Framerate (" + to_string( (int)currentFps ) + "s)###fps counter" ).c_str(), ImGuiTreeNodeFlags_DefaultOpen ) ) {
		ImGui::PlotLines( "##fps_lines", fps.data(), int( fps.size() ), 0, 0, 0.0f, 120.0f, ImVec2( ImGui::GetContentRegionAvailWidth(), 90 ) );
	}
#endif

	static bool sortTimes = false;
	im::Checkbox( "sort times", &sortTimes );
	im::SameLine();
	if( im::Button( "clear timers" ) ) {
		mGpuDurations.clear();
	}

	const float column1Offset = im::GetWindowWidth() - 110;
	im::BeginChild( "##Profile Times", { 0, 0 } );

#if 0
	if( im::CollapsingHeader( "cpu (ms)", nullptr, ImGuiTreeNodeFlags_DefaultOpen ) ) {
		im::Columns( 2, "cpu columns", true );
		im::SetColumnOffset( 1, column1Offset );
		auto cpuTimes = perf::detail::globalCpuProfiler().getElapsedTimes();

		if( sortTimes ) {
			vector<pair<string, double>> sortedTimes( cpuTimes.begin(), cpuTimes.end() );
			stable_sort( sortedTimes.begin(), sortedTimes.end(), [] ( const auto &a, const auto &b ) { return a.second > b.second; } );
			for( const auto &kv : sortedTimes ) {
				displayTimeFn( kv );
			}
		}
		else {
			for( const auto &kv : cpuTimes ) {
				displayTimeFn( kv );
			}
		}
	}
#endif

	im::Columns( 1 );
	if( im::CollapsingHeader( "gpu (ms)", nullptr, ImGuiTreeNodeFlags_DefaultOpen ) ) {
		im::Columns( 2, "gpu columns", true );
		im::SetColumnOffset( 1, column1Offset );

		if( sortTimes ) {
			vector<pair<string, double>> sortedTimes( mGpuDurations.begin(), mGpuDurations.end() );
			stable_sort( sortedTimes.begin(), sortedTimes.end(), [] ( const auto &a, const auto &b ) { return a.second > b.second; } );
			for( const auto &kv : sortedTimes ) {
				displayTimeFn( kv );
			}
		}
		else {
			for( const auto &kv : mGpuDurations ) {
				displayTimeFn( kv );
			}
		}
	}

	im::EndChild();

	im::End(); // "Profiling"
}

} // namespace ju
