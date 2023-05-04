#pragma once

#include "DurationQueryHelper.hpp"

#include <chrono>
#include <memory>
#include <map>

namespace ju {
namespace dg = Diligent;

class Profiler {
public:

    Profiler( dg::IRenderDevice* device );
    ~Profiler();

    void begin( dg::IDeviceContext* context, const std::string &label );
    void end( dg::IDeviceContext* context, const std::string &label );

    //void update( double ElapsedTime );
    void updateUI( bool *open = nullptr );

private:
    // TODO: use for cpu profiling
    using TimePoint = std::chrono::high_resolution_clock::time_point;
    using Seconds  = std::chrono::duration<double>;

    dg::RefCntAutoPtr<dg::IRenderDevice>        mDevice;
    std::unique_ptr<dg::DurationQueryHelper>    mQuerier;
    std::map<std::string, double>               mGpuDurations;

    //std::array<Frame, (1 << NumFramesPOT)> m_FrameHistory = {}; // TODO: keep a history and average for smoother results
};

struct ScopedProfiler {
	ScopedProfiler( const std::string& label, dg::IDeviceContext* context, Profiler *profiler )
		: mLabel( label ), mContext( context ), mProfiler( profiler )
	{
		mProfiler->begin( mContext, mLabel );
	}
	~ScopedProfiler()
	{
		mProfiler->end( mContext, mLabel );
	}
private:
	std::string	            mLabel;
    Profiler*	            mProfiler;
    dg::IDeviceContext*     mContext;
};



// TODO: add macros
} // namespace ju

#define JU_PROFILE( label, ... )	ju::ScopedProfiler __ju_profile{ label, ##__VA_ARGS__ }
