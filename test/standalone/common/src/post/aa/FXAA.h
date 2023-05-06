#pragma once

#include "DeviceContext.h"
#include "Image.h"
#include "RefCntAutoPtr.hpp"
#include "BasicMath.hpp"

namespace ju { namespace aa {

namespace dg = Diligent;
using dg::RefCntAutoPtr;

class FXAA {
public:
	FXAA( const dg::TEXTURE_FORMAT &colorBufferFormat );

	//! Processes \t source's color buffer, placing the result in \t dest.
	// TODO: not sure yet what needs to be in here to draw with the previous render target bound
	void apply();

	void updateUI();

private:
	void initPipelineState( const dg::TEXTURE_FORMAT &colorBufferFormat );

	RefCntAutoPtr<dg::IPipelineState>         mPSO;
	RefCntAutoPtr<dg::IShaderResourceBinding> mSRB;
	RefCntAutoPtr<dg::IBuffer>                mConstantsBuffer;

	struct FxaaConstants {
		float qualitySubpix = 0.75f;
		float qualityEdgeThreshold = 0.033f;
		float padding0;
		float padding1;
	};
	FxaaConstants mFxaaConstants;
};

} } // namespace ju::aa
