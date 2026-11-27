#define WITHD3D
#include "common.h"

#ifdef EXTENDED_COLOURFILTER

#ifndef LIBRW
#error "Need librw for EXTENDED_COLOURFILTER"
#endif

#include "main.h"
#include "RwHelper.h"
#include "Camera.h"
#include "MBlur.h"
#include "postfx.h"
#ifdef MODERN_FEATURES
#include "ModernConfig.h"
#include "ModernPostFXPolicy.h"
#include "ModernBloomPlan.h"
#include "Clock.h"
#endif

RwRaster *CPostFX::pFrontBuffer;
RwRaster *CPostFX::pBackBuffer;
bool CPostFX::bJustInitialised;
int CPostFX::EffectSwitch = POSTFX_NORMAL;
bool CPostFX::BlurOn = false;
bool CPostFX::MotionBlurOn = false;

static RwIm2DVertex Vertex[4];
static RwIm2DVertex Vertex2[4];
static RwImVertexIndex Index[6] = { 0, 1, 2, 0, 2, 3 };

#ifdef RW_D3D9
void *colourfilterVC_PS;
void *contrast_PS;
#ifdef MODERN_FEATURES
static void *modernAA_PS;
static void *modernFXAA_PS;
static void *modernBloom_PS;
#endif
#endif
#ifdef RW_OPENGL
int32 u_blurcolor;
int32 u_contrastAdd;
int32 u_contrastMult;
rw::gl3::Shader *colourFilterVC;
rw::gl3::Shader *contrast;
#ifdef MODERN_FEATURES
static int32 u_modernAA;
static rw::gl3::Shader *modernAA;
static rw::gl3::Shader *modernFXAA;
static rw::gl3::Shader *modernBloom;
static int32 u_bloomBounds, u_bloomTexel, u_bloomParams;
#endif
#endif

void
CPostFX::InitOnce(void)
{
#ifdef RW_OPENGL
	u_blurcolor = rw::gl3::registerUniform("u_blurcolor");
	u_contrastAdd = rw::gl3::registerUniform("u_contrastAdd");
	u_contrastMult = rw::gl3::registerUniform("u_contrastMult");
#ifdef MODERN_FEATURES
	u_modernAA = rw::gl3::registerUniform("u_modernAA");
	u_bloomBounds = rw::gl3::registerUniform("u_bloomBounds");
	u_bloomTexel = rw::gl3::registerUniform("u_bloomTexel");
	u_bloomParams = rw::gl3::registerUniform("u_bloomParams");
#endif
#endif
}

void
CPostFX::Open(RwCamera *cam)
{
	if(pFrontBuffer)
		Close();

	uint32 width  = Pow(2.0f, int32(log2(RwRasterGetWidth (RwCameraGetRaster(cam))))+1);
	uint32 height = Pow(2.0f, int32(log2(RwRasterGetHeight(RwCameraGetRaster(cam))))+1);
	uint32 depth  = RwRasterGetDepth(RwCameraGetRaster(cam));
	pFrontBuffer = RwRasterCreate(width, height, depth, rwRASTERTYPECAMERATEXTURE);
	pBackBuffer = RwRasterCreate(width, height, depth, rwRASTERTYPECAMERATEXTURE);
	bJustInitialised = true;

	float zero, xmax, ymax;

	if(RwRasterGetDepth(RwCameraGetRaster(cam)) == 16){
		zero = HALFPX;
		xmax = width + HALFPX;
		ymax = height + HALFPX;
	}else{
		zero = -HALFPX;
		xmax = width - HALFPX;
		ymax = height - HALFPX;
	}

	RwIm2DVertexSetScreenX(&Vertex[0], zero);
	RwIm2DVertexSetScreenY(&Vertex[0], zero);
	RwIm2DVertexSetScreenZ(&Vertex[0], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[0], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[0], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[0], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[0], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[0], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex[1], zero);
	RwIm2DVertexSetScreenY(&Vertex[1], ymax);
	RwIm2DVertexSetScreenZ(&Vertex[1], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[1], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[1], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[1], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[1], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[1], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex[2], xmax);
	RwIm2DVertexSetScreenY(&Vertex[2], ymax);
	RwIm2DVertexSetScreenZ(&Vertex[2], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[2], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[2], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[2], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[2], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[2], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex[3], xmax);
	RwIm2DVertexSetScreenY(&Vertex[3], zero);
	RwIm2DVertexSetScreenZ(&Vertex[3], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex[3], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex[3], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex[3], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex[3], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex[3], 255, 255, 255, 255);


	RwIm2DVertexSetScreenX(&Vertex2[0], zero + 2.0f);
	RwIm2DVertexSetScreenY(&Vertex2[0], zero + 2.0f);
	RwIm2DVertexSetScreenZ(&Vertex2[0], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex2[0], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex2[0], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex2[0], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex2[0], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex2[0], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex2[1], 2.0f);
	RwIm2DVertexSetScreenY(&Vertex2[1], ymax + 2.0f);
	RwIm2DVertexSetScreenZ(&Vertex2[1], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex2[1], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex2[1], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex2[1], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex2[1], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex2[1], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex2[2], xmax + 2.0f);
	RwIm2DVertexSetScreenY(&Vertex2[2], ymax + 2.0f);
	RwIm2DVertexSetScreenZ(&Vertex2[2], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex2[2], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex2[2], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex2[2], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex2[2], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex2[2], 255, 255, 255, 255);

	RwIm2DVertexSetScreenX(&Vertex2[3], xmax + 2.0f);
	RwIm2DVertexSetScreenY(&Vertex2[3], zero + 2.0f);
	RwIm2DVertexSetScreenZ(&Vertex2[3], RwIm2DGetNearScreenZ());
	RwIm2DVertexSetCameraZ(&Vertex2[3], RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetRecipCameraZ(&Vertex2[3], 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetU(&Vertex2[3], 1.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetV(&Vertex2[3], 0.0f, 1.0f/RwCameraGetNearClipPlane(cam));
	RwIm2DVertexSetIntRGBA(&Vertex2[3], 255, 255, 255, 255);


#ifdef RW_D3D9
#include "shaders/obj/colourfilterVC_PS.inc"
	colourfilterVC_PS = rw::d3d::createPixelShader(colourfilterVC_PS_cso);
#include "shaders/obj/contrastPS.inc"
	contrast_PS = rw::d3d::createPixelShader(contrastPS_cso);
#ifdef MODERN_FEATURES
#include "shaders/obj/modernAA_PS.inc"
	modernAA_PS = rw::d3d::createPixelShader(modernAA_PS_cso);
#include "shaders/obj/modernFXAA_PS.inc"
	modernFXAA_PS = rw::d3d::createPixelShader(modernFXAA_PS_cso);
#include "shaders/obj/modernBloom_PS.inc"
	modernBloom_PS = rw::d3d::createPixelShader(modernBloom_PS_cso);
	// Unsupported shader model or device: leave the effect disabled.
#endif
#endif
#ifdef RW_OPENGL
	using namespace rw::gl3;

	{
#include "shaders/obj/im2d_vert.inc"
#include "shaders/obj/colourfilterVC_frag.inc"
	const char *vs[] = { shaderDecl, header_vert_src, im2d_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, colourfilterVC_frag_src, nil };
	colourFilterVC = Shader::create(vs, fs);
	assert(colourFilterVC);
	}

	{
#include "shaders/obj/im2d_vert.inc"
#include "shaders/obj/contrast_frag.inc"
	const char *vs[] = { shaderDecl, header_vert_src, im2d_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, contrast_frag_src, nil };
	contrast = Shader::create(vs, fs);
	assert(contrast);
	}
#ifdef MODERN_FEATURES
	{
#include "shaders/obj/im2d_vert.inc"
#include "shaders/obj/modernAA_frag.inc"
	const char *vs[] = { shaderDecl, header_vert_src, im2d_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernAA_frag_src, nil };
	modernAA = Shader::create(vs, fs); // nil means graceful fallback
	}
	{
#include "shaders/obj/im2d_vert.inc"
#include "shaders/obj/modernFXAA_frag.inc"
	const char *vs[] = { shaderDecl, header_vert_src, im2d_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernFXAA_frag_src, nil };
	modernFXAA = Shader::create(vs, fs);
	}
	{
#include "shaders/obj/im2d_vert.inc"
#include "shaders/obj/modernBloom_frag.inc"
	const char *vs[] = { shaderDecl, header_vert_src, im2d_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernBloom_frag_src, nil };
	modernBloom = Shader::create(vs, fs);
	}
#endif

#endif
}

#if defined(MODERN_FEATURES) && (defined(RW_OPENGL) || defined(RW_D3D9))
static void CloseModernBloom(void);
#endif

void
CPostFX::Close(void)
{
#if defined(MODERN_FEATURES) && (defined(RW_OPENGL) || defined(RW_D3D9))
	CloseModernBloom();
#endif
	if(pFrontBuffer){
		RwRasterDestroy(pFrontBuffer);
		pFrontBuffer = nil;
	}
	if(pBackBuffer){
		RwRasterDestroy(pBackBuffer);
		pBackBuffer = nil;
	}
#ifdef RW_D3D9
#ifdef MODERN_FEATURES
	if(modernAA_PS){
		rw::d3d::destroyPixelShader(modernAA_PS);
		modernAA_PS = nil;
	}
	if(modernFXAA_PS){
		rw::d3d::destroyPixelShader(modernFXAA_PS);
		modernFXAA_PS = nil;
	}
	if(modernBloom_PS){
		rw::d3d::destroyPixelShader(modernBloom_PS);
		modernBloom_PS = nil;
	}
#endif
	if(colourfilterVC_PS){
		rw::d3d::destroyPixelShader(colourfilterVC_PS);
		colourfilterVC_PS = nil;
	}
	if(contrast_PS){
		rw::d3d::destroyPixelShader(contrast_PS);
		contrast_PS = nil;
	}
#endif
#ifdef RW_OPENGL
#ifdef MODERN_FEATURES
	if(modernAA){
		modernAA->destroy();
		modernAA = nil;
	}
	if(modernFXAA){
		modernFXAA->destroy();
		modernFXAA = nil;
	}
	if(modernBloom){
		modernBloom->destroy();
		modernBloom = nil;
	}
#endif
	if(colourFilterVC){
		colourFilterVC->destroy();
		colourFilterVC = nil;
	}
	if(contrast){
		contrast->destroy();
		contrast = nil;
	}
#endif
}

void
CPostFX::RenderOverlayBlur(RwCamera *cam, int32 r, int32 g, int32 b, int32 a)
{
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pFrontBuffer);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

	RwIm2DVertexSetIntRGBA(&Vertex[0], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex[1], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex[2], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex[3], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex2[0], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex2[1], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex2[2], r*2, g*2, b*2, 30);
	RwIm2DVertexSetIntRGBA(&Vertex2[3], r*2, g*2, b*2, 30);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, BlurOn ? Vertex2 : Vertex, 4, Index, 6);


	RwIm2DVertexSetIntRGBA(&Vertex2[0], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex[0], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex2[1], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex[1], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex2[2], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex[2], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex2[3], r, g, b, a);
	RwIm2DVertexSetIntRGBA(&Vertex[3], r, g, b, a);

	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);

	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, BlurOn ? Vertex2 : Vertex, 4, Index, 6);
}

void
CPostFX::RenderOverlaySniper(RwCamera *cam, int32 r, int32 g, int32 b, int32 a)
{
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pFrontBuffer);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);

	RwIm2DVertexSetIntRGBA(&Vertex[0], r, g, b, 80);
	RwIm2DVertexSetIntRGBA(&Vertex[1], r, g, b, 80);
	RwIm2DVertexSetIntRGBA(&Vertex[2], r, g, b, 80);
	RwIm2DVertexSetIntRGBA(&Vertex[3], r, g, b, 80);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);
}

float CPostFX::Intensity = 1.0f;

void
CPostFX::RenderOverlayShader(RwCamera *cam, int32 r, int32 g, int32 b, int32 a)
{
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pBackBuffer);

	if(EffectSwitch == POSTFX_MOBILE){
		float mult[3], add[3];
		mult[0] = (r-64)/256.0f + 1.4f;
		mult[1] = (g-64)/256.0f + 1.4f;
		mult[2] = (b-64)/256.0f + 1.4f;
		add[0] = r/1536.f - 0.05f;
		add[1] = g/1536.f - 0.05f;
		add[2] = b/1536.f - 0.05f;
#ifdef RW_D3D9
		rw::d3d::d3ddevice->SetPixelShaderConstantF(10, mult, 1);
		rw::d3d::d3ddevice->SetPixelShaderConstantF(11, add, 1);

		rw::d3d::im2dOverridePS = contrast_PS;
#endif
#ifdef RW_OPENGL
		rw::gl3::im2dOverrideShader = contrast;
		contrast->use();
		glUniform3fv(contrast->uniformLocations[u_contrastMult], 1, mult);
		glUniform3fv(contrast->uniformLocations[u_contrastAdd], 1, add);
#endif
	}else{
		float f = Intensity;
		float blurcolors[4];
		blurcolors[0] = r*f/255.0f;
		blurcolors[1] = g*f/255.0f;
		blurcolors[2] = b*f/255.0f;
		blurcolors[3] = 30/255.0f;
#ifdef RW_D3D9
		rw::d3d::d3ddevice->SetPixelShaderConstantF(10, blurcolors, 1);
		rw::d3d::im2dOverridePS = colourfilterVC_PS;
#endif
#ifdef RW_OPENGL
		rw::gl3::im2dOverrideShader = colourFilterVC;
		colourFilterVC->use();
		glUniform4fv(colourFilterVC->uniformLocations[u_blurcolor], 1, blurcolors);
#endif
	}
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);
#ifdef RW_D3D9
	rw::d3d::im2dOverridePS = nil;
#endif
#ifdef RW_OPENGL
	rw::gl3::im2dOverrideShader = nil;
#endif
}

void
CPostFX::RenderMotionBlur(RwCamera *cam, uint32 blur)
{
	if(blur == 0)
		return;

	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pFrontBuffer);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	RwIm2DVertexSetIntRGBA(&Vertex[0], 255, 255, 255, blur);
	RwIm2DVertexSetIntRGBA(&Vertex[1], 255, 255, 255, blur);
	RwIm2DVertexSetIntRGBA(&Vertex[2], 255, 255, 255, blur);
	RwIm2DVertexSetIntRGBA(&Vertex[3], 255, 255, 255, blur);

	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);
}

bool
CPostFX::NeedBackBuffer(void)
{
	// Current frame -- needed for non-blur effect
	switch(EffectSwitch){
	case POSTFX_OFF:
	case POSTFX_SIMPLE:
		// no actual rendering here
		return false;
	case POSTFX_NORMAL:
		if(MotionBlurOn)
			return false;
		else
			return true;
	case POSTFX_MOBILE:
		return true;
	}
	return false;
}

bool
CPostFX::NeedFrontBuffer(int32 type)
{
	// Last frame -- needed for motion blur
	if(CMBlur::Drunkness > 0.0f)
		return true;
	if(type == MOTION_BLUR_SNIPER)
		return true;

	switch(EffectSwitch){
	case POSTFX_OFF:
	case POSTFX_SIMPLE:
		// no actual rendering here
		return false;
	case POSTFX_NORMAL:
		if(MotionBlurOn)
			return true;
		else
			return false;
	case POSTFX_MOBILE:
		return false;
	}
	return false;
}

void
CPostFX::GetBackBuffer(RwCamera *cam)
{
	RwRasterPushContext(pBackBuffer);
	RwRasterRenderFast(RwCameraGetRaster(cam), 0, 0);
	RwRasterPopContext();
}

#if defined(MODERN_FEATURES) && (defined(RW_OPENGL) || defined(RW_D3D9))
// These targets are color-only and owned by the postfx lifecycle. The scene
// camera is ended and restored around each target switch, including failures.
static RwRaster *bloomTargets[3];
static int bloomWidth, bloomHeight, bloomFailedWidth, bloomFailedHeight;

static void
CloseModernBloom(void)
{
	for(int i = 0; i < 3; ++i){
		if(bloomTargets[i]) RwRasterDestroy(bloomTargets[i]);
		bloomTargets[i] = nil;
	}
	bloomWidth = bloomHeight = bloomFailedWidth = bloomFailedHeight = 0;
}

static bool
BloomRasterAvailable(RwRaster *raster)
{
	if(!raster) return false;
#ifdef RW_D3D9
	return GETD3DRASTEREXT(raster)->texture != nil;
#else
	return true; // GL allocation is checked for framebuffer completeness below.
#endif
}

static bool
BloomTargetBound(RwRaster *raster)
{
#ifdef RW_OPENGL
	(void)raster;
	return glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
#else
	if(!BloomRasterAvailable(raster)) return false;
	IDirect3DSurface9 *expected = nil, *actual = nil;
	IDirect3DTexture9 *texture = (IDirect3DTexture9*)GETD3DRASTEREXT(raster)->texture;
	bool ok = SUCCEEDED(texture->GetSurfaceLevel(0, &expected)) && expected &&
	          SUCCEEDED(rw::d3d::d3ddevice->GetRenderTarget(0, &actual)) && actual == expected;
	if(actual) actual->Release();
	if(expected) expected->Release();
	return ok;
#endif
}

static bool
EnsureModernBloom(int width, int height)
{
	if(bloomWidth == width && bloomHeight == height &&
	   BloomRasterAvailable(bloomTargets[0]) && BloomRasterAvailable(bloomTargets[1]) &&
	   BloomRasterAvailable(bloomTargets[2]))
		return true;
	if(bloomFailedWidth == width && bloomFailedHeight == height)
		return false;
	CloseModernBloom();
	int maxSize;
#ifdef RW_OPENGL
	maxSize = rw::gl3::maxTextureSize;
#else
	D3DCAPS9 caps;
	if(FAILED(rw::d3d::d3ddevice->GetDeviceCaps(&caps))) return false;
	maxSize = int(Min(caps.MaxTextureWidth, caps.MaxTextureHeight));
#endif
	ModernBloomPlan::Plan plan;
	// A conservative lower-bound budget; actual driver memory can be higher.
	if(!ModernBloomPlan::Build(width, height, maxSize, 24u * 1024u * 1024u, &plan)){
		bloomFailedWidth = width; bloomFailedHeight = height;
		return false;
	}
	for(int i = 0; i < 3; ++i){
		bloomTargets[i] = RwRasterCreate(plan.levels[i].width, plan.levels[i].height,
		                                32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888);
		if(!BloomRasterAvailable(bloomTargets[i])){
			CloseModernBloom();
			bloomFailedWidth = width; bloomFailedHeight = height;
			return false;
		}
	}
	bloomWidth = width; bloomHeight = height;
	return true;
}

// Draw the entire valid source rectangle, keeping all samples inside it.
static void
DrawBloomQuad(RwCamera *cam, RwRaster *source, int validWidth, int validHeight,
              int kind, float intensity)
{
	int tw = RwRasterGetWidth(source), th = RwRasterGetHeight(source);
	int dw = RwRasterGetWidth(RwCameraGetRaster(cam));
	int dh = RwRasterGetHeight(RwCameraGetRaster(cam));
	float bounds[4];
#ifdef RW_OPENGL
	int y = th - validHeight;
	float vTop = 1.0f, vBottom = 1.0f - float(validHeight) / th;
#else
	int y = 0;
	float vTop = 0.0f, vBottom = float(validHeight) / th;
#endif
	if(!ModernBloomPlan::Bounds(tw, th, 0, y, validWidth, validHeight, bounds)) return;
	float texel[4] = { 1.0f/tw, 1.0f/th, kind == 0 ? 0.5f : 1.0f, 0.0f };
	float params[4] = { 0.78f, 0.10f, float(kind), intensity };
	float uRight = float(validWidth) / tw;
	float recip = 1.0f / RwCameraGetNearClipPlane(cam);
	float xs[4] = { -HALFPX, -HALFPX, float(dw)-HALFPX, float(dw)-HALFPX };
	float ys[4] = { -HALFPX, float(dh)-HALFPX, float(dh)-HALFPX, -HALFPX };
	float us[4] = { 0.0f, 0.0f, uRight, uRight };
	float vs[4] = { vTop, vBottom, vBottom, vTop };
	RwIm2DVertex verts[4];
	for(int i = 0; i < 4; ++i){
		RwIm2DVertexSetScreenX(&verts[i], xs[i]);
		RwIm2DVertexSetScreenY(&verts[i], ys[i]);
		RwIm2DVertexSetScreenZ(&verts[i], RwIm2DGetNearScreenZ());
		RwIm2DVertexSetCameraZ(&verts[i], RwCameraGetNearClipPlane(cam));
		RwIm2DVertexSetRecipCameraZ(&verts[i], recip);
		RwIm2DVertexSetU(&verts[i], us[i], recip);
		RwIm2DVertexSetV(&verts[i], vs[i], recip);
		RwIm2DVertexSetIntRGBA(&verts[i], 255, 255, 255, 255);
	}
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, source);
#ifdef RW_OPENGL
	rw::gl3::im2dOverrideShader = modernBloom;
	modernBloom->use();
	glUniform4fv(modernBloom->uniformLocations[u_bloomBounds], 1, bounds);
	glUniform4fv(modernBloom->uniformLocations[u_bloomTexel], 1, texel);
	glUniform4fv(modernBloom->uniformLocations[u_bloomParams], 1, params);
#else
	rw::d3d::im2dOverridePS = modernBloom_PS;
	rw::d3d::d3ddevice->SetPixelShaderConstantF(10, bounds, 1);
	rw::d3d::d3ddevice->SetPixelShaderConstantF(11, texel, 1);
	rw::d3d::d3ddevice->SetPixelShaderConstantF(12, params, 1);
#endif
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, verts, 4, Index, 6);
#ifdef RW_OPENGL
	rw::gl3::im2dOverrideShader = nil;
#else
	rw::d3d::im2dOverridePS = nil;
#endif
}
#endif

#ifdef MODERN_FEATURES
void
CPostFX::RenderModernBloom(RwCamera *cam)
{
#if defined(RW_OPENGL) || defined(RW_D3D9)
	if(!CModernConfig::m_bMasterEnable || CModernConfig::m_nNeonBloom <= 0 ||
	   !cam || !RwCameraGetRaster(cam) || !pBackBuffer || RwCameraGetNearClipPlane(cam) <= 0.0f)
		return;
#ifdef RW_OPENGL
	if(!modernBloom) return;
#ifdef LIBRW_SDL2
	if(RwEngineGetRenderScale() < 100) return;
#endif
#else
	if(!modernBloom_PS) return;
#endif
	RwRaster *scene = RwCameraGetRaster(cam);
	RwRaster *depth = RwCameraGetZRaster(cam);
	int width = RwRasterGetWidth(scene), height = RwRasterGetHeight(scene);
	float capture[4];
	if(!ModernPostFXPolicy::CaptureParams(width, height, RwRasterGetWidth(pBackBuffer),
	                                      RwRasterGetHeight(pBackBuffer), capture))
		return;
#ifdef RW_D3D9
	if(!BloomRasterAvailable(pBackBuffer)) return;
#endif
	// Do not capture or switch targets on every frame after an allocation error.
	if(bloomFailedWidth == width && bloomFailedHeight == height) return;
	GetBackBuffer(cam);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	RwCameraEndUpdate(cam);
	bool ok = EnsureModernBloom(width, height);
	RwCameraSetZRaster(cam, nil);
	for(int i = 0; ok && i < 5; ++i){
		ModernBloomPlan::Pass pass = ModernBloomPlan::PassAt(i);
		RwRaster *source = pass.source < 0 ? pBackBuffer : bloomTargets[pass.source];
		RwRaster *destination = bloomTargets[pass.destination];
		if(!BloomRasterAvailable(source) || !BloomRasterAvailable(destination)){
			ok = false;
			break;
		}
		RwCameraSetRaster(cam, destination);
		RwCameraBeginUpdate(cam);
		if(!BloomTargetBound(destination)){
			RwCameraEndUpdate(cam);
			ok = false;
			break;
		}
		DefinedState();
		RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
		RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
		RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
		DrawBloomQuad(cam, source, pass.source < 0 ? width : RwRasterGetWidth(source),
		              pass.source < 0 ? height : RwRasterGetHeight(source), pass.kind, 1.0f);
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
		RwCameraEndUpdate(cam);
	}
	RwCameraSetRaster(cam, scene);
	RwCameraSetZRaster(cam, depth);
	RwCameraBeginUpdate(cam);
	if(ok){
		DefinedState();
		RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)TRUE);
		RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDONE);
		RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
		RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
		float amount = CModernConfig::m_nNeonBloom >= 2 ? 0.75f : 0.4f;
		amount *= ModernBloomPlan::NightFactor(CClock::GetHours(), CClock::GetMinutes());
		DrawBloomQuad(cam, bloomTargets[0], RwRasterGetWidth(bloomTargets[0]),
		              RwRasterGetHeight(bloomTargets[0]), 2, amount);
		RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	}else{
		CloseModernBloom();
		bloomFailedWidth = width; bloomFailedHeight = height;
	}
	DefinedState();
#else
	(void)cam;
#endif
}
#endif

#ifdef MODERN_FEATURES
void
CPostFX::RenderExperimentalEdgeAA(RwCamera *cam)
{
#if defined(RW_OPENGL) || defined(RW_D3D9)
	if (!CModernConfig::m_bMasterEnable ||
	    (!CModernConfig::m_bFxaa && !CModernConfig::m_bExperimentalEdgeAA) ||
	    !cam || !RwCameraGetRaster(cam) || !pBackBuffer)
		return;
#if defined(RW_OPENGL) && defined(LIBRW_SDL2)
	// The GL raster-copy path still uses unscaled camera dimensions. Avoid
	// sampling outside the scaled framebuffer until that path is corrected.
	if (RwEngineGetRenderScale() < 100)
		return;
#endif
#ifdef RW_OPENGL
	// Prefer FXAA 3.11; the older preview is an independent opt-in fallback.
	rw::gl3::Shader *shader = CModernConfig::m_bFxaa ? modernFXAA : nil;
	if (!shader && CModernConfig::m_bExperimentalEdgeAA) shader = modernAA;
	if (!shader) return;
#else
	void *shader = CModernConfig::m_bFxaa ? modernFXAA_PS : nil;
	if (!shader && CModernConfig::m_bExperimentalEdgeAA) shader = modernAA_PS;
	if (!shader) return;
#endif
	int width = RwRasterGetWidth(RwCameraGetRaster(cam));
	int height = RwRasterGetHeight(RwCameraGetRaster(cam));
	int texWidth = RwRasterGetWidth(pBackBuffer);
	int texHeight = RwRasterGetHeight(pBackBuffer);
	float params[4];
	if (!ModernPostFXPolicy::CaptureParams(width, height, texWidth, texHeight, params))
		return; // framebuffer has changed or the capture isn't usable

	// Capture after world effects and motion blur, before HUD/menu rendering.
	GetBackBuffer(cam);
	DefinedState();
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTUREADDRESS, (void*)rwTEXTUREADDRESSCLAMP);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERLINEAR);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, pBackBuffer);
	for (int i = 0; i < 4; i++)
		RwIm2DVertexSetIntRGBA(&Vertex[i], 255, 255, 255, 255);
#ifdef RW_OPENGL
	rw::gl3::im2dOverrideShader = shader;
	shader->use();
	glUniform4fv(shader->uniformLocations[u_modernAA], 1, params);
#else
	rw::d3d::d3ddevice->SetPixelShaderConstantF(10, params, 1);
	rw::d3d::im2dOverridePS = shader;
#endif
	RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, Vertex, 4, Index, 6);
#ifdef RW_OPENGL
	rw::gl3::im2dOverrideShader = nil;
#else
	rw::d3d::im2dOverridePS = nil;
#endif
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	DefinedState();
#else
	(void)cam;
#endif
}
#endif

void
CPostFX::Render(RwCamera *cam, uint32 red, uint32 green, uint32 blue, uint32 blur, int32 type, uint32 bluralpha)
{
	PUSH_RENDERGROUP("CPostFX::Render");

	if(pFrontBuffer == nil)
		Open(cam);
	assert(pFrontBuffer);
	assert(pBackBuffer);

	if(type == MOTION_BLUR_LIGHT_SCENE){
		SmoothColor(red, green, blue, blur);
		red = AvgRed;
		green = AvgGreen;
		blue = AvgBlue;
		blur = AvgAlpha;
	}

	if(NeedBackBuffer())
		GetBackBuffer(cam);

	DefinedState();

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void*)rwFILTERNEAREST);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)FALSE);

	if(type == MOTION_BLUR_SNIPER){
		if(!bJustInitialised)
			RenderOverlaySniper(cam, red, green, blue, blur);
	}else switch(EffectSwitch){
	case POSTFX_OFF:
	case POSTFX_SIMPLE:
		// no actual rendering here
		break;
	case POSTFX_NORMAL:
		if(MotionBlurOn){
			if(!bJustInitialised)
				RenderOverlayBlur(cam, red, green, blue, blur);
		}else{
			RenderOverlayShader(cam, red, green, blue, blur);
		}
		break;
	case POSTFX_MOBILE:
		RenderOverlayShader(cam, red, green, blue, blur);
		break;
	}

	if(!bJustInitialised)
		RenderMotionBlur(cam, 175.0f * CMBlur::Drunkness);

	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)TRUE);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, nil);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)FALSE);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);

	if(NeedFrontBuffer(type)){
		RwRasterPushContext(pFrontBuffer);
		RwRasterRenderFast(RwCameraGetRaster(cam), 0, 0);
		RwRasterPopContext();
		bJustInitialised = false;
	}else
		bJustInitialised = true;

	POP_RENDERGROUP();
}

int CPostFX::PrevRed[NUMAVERAGE], CPostFX::AvgRed;
int CPostFX::PrevGreen[NUMAVERAGE], CPostFX::AvgGreen;
int CPostFX::PrevBlue[NUMAVERAGE], CPostFX::AvgBlue;
int CPostFX::PrevAlpha[NUMAVERAGE], CPostFX::AvgAlpha;
int CPostFX::Next;
int CPostFX::NumValues;

// This is rather annoying...the blur color can flicker slightly
// which becomes very visible when amplified by the shader
void
CPostFX::SmoothColor(uint32 red, uint32 green, uint32 blue, uint32 alpha)
{
	PrevRed[Next] = red;
	PrevGreen[Next] = green;
	PrevBlue[Next] = blue;
	PrevAlpha[Next] = alpha;
	Next = (Next+1) % NUMAVERAGE;
	NumValues = Min(NumValues+1, NUMAVERAGE);

	AvgRed = 0;
	AvgGreen = 0;
	AvgBlue = 0;
	AvgAlpha = 0;
	for(int i = 0; i < NumValues; i++){
		AvgRed += PrevRed[i];
		AvgGreen += PrevGreen[i];
		AvgBlue += PrevBlue[i];
		AvgAlpha += PrevAlpha[i];
	}
	AvgRed /= NumValues;
	AvgGreen /= NumValues;
	AvgBlue /= NumValues;
	AvgAlpha /= NumValues;
}

#endif
