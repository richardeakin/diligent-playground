
#pragma once

//#include "RenderDevice.h"
#include "RefCntAutoPtr.hpp"
#include "Buffer.h"
#include "Image.h"
#include "DeviceContext.h"
#include "BasicMath.hpp"

#include "../../common/src/FileWatch.h"
#include <filesystem>

namespace ju {

namespace dg = Diligent;
namespace fs = std::filesystem;

enum VERTEX_COMPONENT_FLAGS : int
{
	VERTEX_COMPONENT_FLAG_NONE     = 0x00,
	VERTEX_COMPONENT_FLAG_POSITION = 0x01,
	VERTEX_COMPONENT_FLAG_NORMAL   = 0x02,
	VERTEX_COMPONENT_FLAG_TEXCOORD = 0x04,

	VERTEX_COMPONENT_FLAG_POS_UV =
	VERTEX_COMPONENT_FLAG_POSITION |
	VERTEX_COMPONENT_FLAG_TEXCOORD,

	VERTEX_COMPONENT_FLAG_POS_NORM_UV =
	VERTEX_COMPONENT_FLAG_POSITION |
	VERTEX_COMPONENT_FLAG_NORMAL |
	VERTEX_COMPONENT_FLAG_TEXCOORD
};
DEFINE_FLAG_ENUM_OPERATORS(VERTEX_COMPONENT_FLAGS);

struct StaticShaderVar {
	dg::SHADER_TYPE		shaderType = dg::SHADER_TYPE_UNKNOWN;
	const dg::Char*		name = nullptr;
	dg::IDeviceObject*	object = nullptr;
};

struct ShaderResourceVar {
	dg::ShaderResourceVariableDesc	desc;
	dg::IDeviceObject*				object = nullptr;
};

class Cube {
public:
	struct Options {
		fs::path				vertPath;
		fs::path				pixelPath;
		std::string				name;
		VERTEX_COMPONENT_FLAGS	components = VERTEX_COMPONENT_FLAG_POS_UV;

		std::vector<ShaderResourceVar>	shaderResourceVars;
		std::vector<StaticShaderVar>	staticShaderVars;

		//std::function<void>	oReInitFn;
	};
	Cube( const Options &options = Options() );

	// this is used for the structured buffer (ParticleAttribs), which is set after the SRB is already constructed.
	//void setShaderResourceVar( dg::SHADER_TYPE shaderType, const dg::Char* name, dg::IDeviceObject* object );

	void update( double deltaSeconds );
	void render( dg::IDeviceContext* context, const dg::float4x4 &mvp, uint32_t numInstances = 1 );

	void setTransform( const dg::float4x4 &m )	{ mTransform = m; }

	void setLightDir( const dg::float3 &dir )	{ mLightDirection = dir; }

private:
	void initPipelineState();
	void initVertexBuffer();
	void initIndexBuffer();

	void watchShadersDir();
	void reloadOnAssetsUpdated();

	dg::RefCntAutoPtr<dg::IPipelineState>         mPSO;
	dg::RefCntAutoPtr<dg::IShaderResourceBinding> mSRB;
	dg::RefCntAutoPtr<dg::IBuffer>                mVertexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                mIndexBuffer;
	dg::RefCntAutoPtr<dg::IBuffer>                mVSConstants;

	Options	mOptions;

	dg::float3      mLightDirection  = dg::float3(0, 1, 0); // TODO: this should be part of a global constants buffer
	dg::float4x4	mTransform;

	ju::FileWatchHandle     mShadersDirWatchHandle;
	bool                    mShaderAssetsMarkedDirty = false;
};

} // namespace ju