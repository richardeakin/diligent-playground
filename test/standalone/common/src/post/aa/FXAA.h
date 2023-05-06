#pragma once

#include "Image.h"
#include "RefCntAutoPtr.hpp"

namespace ju { namespace aa {

namespace dg = Diligent;
using dg::RefCntAutoPtr;

class FXAA {
public:
	FXAA();

	//! Processes \t source's color buffer, placing the result in \t dest.
	// TODO: not sure yet what needs to be in here to draw with the previous render target bound
	void apply();

private:
};

} } // namespace ju::aa
