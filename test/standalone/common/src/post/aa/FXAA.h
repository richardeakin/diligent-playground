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

	//! Set the texture that will get antialiased
	void setTexture( dg::ITextureView* textureView );

	//! Processes \t source's color buffer, placing the result in \t dest.
	// TODO: not sure yet what needs to be in here to draw with the previous render target bound
	void apply( dg::IDeviceContext* context, dg::ITextureView *texture );

	void updateUI();

private:
	void initPipelineState( const dg::TEXTURE_FORMAT &colorBufferFormat );
	void watchShadersDir();
	void reloadOnAssetsUpdated();

	RefCntAutoPtr<dg::IPipelineState>         mPSO;
	RefCntAutoPtr<dg::IShaderResourceBinding> mSRB;
	RefCntAutoPtr<dg::IBuffer>                mConstantsBuffer;

	struct FxaaConstants {
		float qualitySubpix = 0.75f;
		float qualityEdgeThreshold = 0.033f;
		float padding0;
		float padding1;
	};
	static_assert(sizeof(FxaaConstants) % 16 == 0, "must be aligned to 16 bytes");
	
	FxaaConstants mFxaaConstants;
};

} } // namespace ju::aa
