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

// TODO: add ScopedProfiler + macros

} // namespace ju
