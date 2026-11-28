#include "common.h"

#ifdef RW_OPENGL
#include "main.h"
#include "RwHelper.h"
#include "Lights.h"
#include "Timecycle.h"
#include "FileMgr.h"
#include "Clock.h"
#include "Weather.h"
#include "TxdStore.h"
#include "Renderer.h"
#include "World.h"
#include "custompipes.h"
#ifdef MODERN_FEATURES
#include "ModernConfig.h"
#include "ModernLighting.h"
#include "ModernRenderDiagnostics.h"
#if defined(RW_GL3)
#include "DirectionalShadowPlan.h"
#include "Entity.h"
#include "Game.h"
#include "CutsceneMgr.h"
#include "rpskin.h"
#endif
#endif

#ifdef EXTENDED_PIPELINES

#ifndef LIBRW
#error "Need librw for EXTENDED_PIPELINES"
#endif

namespace CustomPipes {

static int32 u_viewVec;
static int32 u_rampStart;
static int32 u_rampEnd;
static int32 u_rimData;

static int32 u_lightMap;

static int32 u_eye;
static int32 u_reflProps;
static int32 u_specDir;
static int32 u_specColor;
#ifdef MODERN_FEATURES
static int32 u_modernPosition, u_modernColor, u_modernDirection, u_modernOptions, u_modernSky;
static rw::gl3::Shader *modernWorldShader, *modernPedShader, *modernPedSkinShader, *modernVehicleShader;
static bool modernRenderingRoad;
#if defined(RW_GL3)
static int32 u_shadowMatrix, u_shadowParams;
static rw::gl3::Shader *directionalShadowShader;
static RwCamera *directionalShadowCamera;
static RwTexture *directionalShadowTexture;
static int directionalShadowSize, directionalShadowFailedSize;
static bool directionalShadowValid, directionalShadowHasBasis;
static DirectionalShadowFit::Vec3 directionalShadowRight;
static DirectionalShadowFit::Result directionalShadowFit;
static float directionalShadowMatrix[16], directionalShadowStrength;
#endif
#endif

#define U(i) currentShader->uniformLocations[i]

#ifdef MODERN_FEATURES
static void
UploadModernLights(rw::Atomic *atomic)
{
	using namespace rw::gl3;
	ModernLightUniforms lights;
	BuildModernLights(atomic, true, lights);
	glUniform4fv(U(u_modernPosition), ModernLightingPolicy::MaxLights, &lights.position[0][0]);
	glUniform4fv(U(u_modernColor), ModernLightingPolicy::MaxLights, &lights.color[0][0]);
	glUniform4fv(U(u_modernDirection), ModernLightingPolicy::MaxLights, &lights.direction[0][0]);
}
static bool
UseModernLights(rw::Atomic *atomic, rw::gl3::Shader *shader)
{
	return shader && CModernConfig::m_bMasterEnable && CModernConfig::m_bDynamicPointLights &&
	       !bRenderingEnvMap && (atomic->geometry->flags & rw::Geometry::NORMALS);
}
#endif

#if defined(MODERN_FEATURES) && defined(RW_GL3)
// The first GL/GLES implementation uses the existing camera-texture + depth
// attachment path. Depth is packed into RGB8 because GLES z rasters are
// renderbuffers, not sampleable textures. Classic shadows are never disabled.
static void
DestroyDirectionalShadowResources(void)
{
	directionalShadowValid = false;
	if(directionalShadowCamera){
		RwFrame *frame = RwCameraGetFrame(directionalShadowCamera);
		RwRaster *z = RwCameraGetZRaster(directionalShadowCamera);
		RwRaster *color = RwCameraGetRaster(directionalShadowCamera);
		GLint previousFbo = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFbo);
		GLuint shadowFbo = color ? PLUGINOFFSET(rw::gl3::Gl3Raster, color, rw::gl3::nativeRasterOffset)->fbo : 0;
		RwCameraSetFrame(directionalShadowCamera, nil);
		RwCameraSetZRaster(directionalShadowCamera, nil);
		RwCameraSetRaster(directionalShadowCamera, nil);
		if(frame) RwFrameDestroy(frame);
		if(z) RwRasterDestroy(z);
		// Destroying a paired z raster binds its color FBO. Do not delete a
		// bound FBO behind librw's framebuffer-binding cache.
		rw::gl3::bindFramebuffer(previousFbo == (GLint)shadowFbo ? 0 : (GLuint)previousFbo);
		if(directionalShadowTexture){
			RwTextureSetRaster(directionalShadowTexture, nil);
			RwTextureDestroy(directionalShadowTexture);
			directionalShadowTexture = nil;
		}
		if(color) RwRasterDestroy(color);
		RwCameraDestroy(directionalShadowCamera);
		directionalShadowCamera = nil;
	}
	directionalShadowSize = 0;
	directionalShadowHasBasis = false;
}

static bool
CreateDirectionalShadowResources(int size)
{
	if(directionalShadowCamera && directionalShadowSize == size) return true;
	DestroyDirectionalShadowResources();
	if(directionalShadowFailedSize == size) return false;
	GLint maxSize = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
	if(maxSize < size){ directionalShadowFailedSize = size; return false; }
	directionalShadowCamera = RwCameraCreate();
	if(!directionalShadowCamera) goto fail;
	RwCameraSetFrame(directionalShadowCamera, RwFrameCreate());
	if(!RwCameraGetFrame(directionalShadowCamera)) goto fail;
	RwCameraSetZRaster(directionalShadowCamera, RwRasterCreate(size, size, 0, rwRASTERTYPEZBUFFER));
	if(!RwCameraGetZRaster(directionalShadowCamera)) goto fail;
	RwCameraSetRaster(directionalShadowCamera, RwRasterCreate(size, size, 32, rwRASTERTYPECAMERATEXTURE | rwRASTERFORMAT8888));
	if(!RwCameraGetRaster(directionalShadowCamera)) goto fail;
	directionalShadowTexture = RwTextureCreate(RwCameraGetRaster(directionalShadowCamera));
	if(!directionalShadowTexture) goto fail;
	RwTextureSetAddressing(directionalShadowTexture, rwTEXTUREADDRESSCLAMP);
	RwTextureSetFilterMode(directionalShadowTexture, rwFILTERNEAREST);
	RwCameraSetProjection(directionalShadowCamera, rwPARALLEL);
	directionalShadowSize = size;
	directionalShadowFailedSize = 0;
	return true;
fail:
	DestroyDirectionalShadowResources();
	directionalShadowFailedSize = size;
	return false;
}

struct ShadowCandidates {
	DirectionalShadowPlan::CasterCollector<512> collector;
	DirectionalShadowFit::Vec3 eye;
};

static void
CollectShadowCandidate(CEntity *entity, void *data)
{
	if(!entity || !entity->m_rwObject || !entity->bIsVisible || entity->bDoNotRender ||
	   entity->bDistanceFade || RwObjectGetType(entity->m_rwObject) != rpATOMIC) return;
	rw::Atomic *atomic = (rw::Atomic*)entity->m_rwObject;
	if(!(atomic->getFlags() & rw::Atomic::RENDER) || !atomic->geometry || !atomic->getFrame() ||
	   rw::Skin::get(atomic->geometry)) return;
	const rw::Sphere *sphere = atomic->getWorldBoundingSphere();
	if(!sphere) return;
	ShadowCandidates *candidates = (ShadowCandidates*)data;
	std::size_t id = reinterpret_cast<std::size_t>(entity);
	for(std::size_t i = 0; i < candidates->collector.count; ++i)
		if(candidates->collector.entries[i].id == id) return;
	DirectionalShadowFit::Vec3 center = {sphere->center.x, sphere->center.y, sphere->center.z};
	candidates->collector.AddCandidate(directionalShadowFit, candidates->eye, center, sphere->radius, id);
}

static void
DrawShadowAtomic(rw::Atomic *atomic)
{
	using namespace rw;
	using namespace rw::gl3;
	rw::ObjPipeline *pipe = atomic->getPipeline();
	if(!pipe || pipe->platform != PLATFORM_GL3 || !pipe->impl.instance) return;
	pipe->instance(atomic);
	if(!atomic->geometry->instData || atomic->geometry->instData->platform != PLATFORM_GL3) return;
	rw::gl3::InstanceDataHeader *header = (rw::gl3::InstanceDataHeader*)atomic->geometry->instData;
	setWorldMatrix(atomic->getFrame()->getLTM());
	setupVertexInput(header);
	for(uint32 i = 0; i < header->numMeshes; ++i){
		InstanceData *inst = &header->inst[i];
		Material *mat = inst->material;
		// Translucent meshes are not opaque occluders. Texture cutouts are
		// alpha-tested in the shader; vertex-faded meshes are excluded.
		if(!mat || mat->color.alpha != 255 || inst->vertexAlpha) continue;
		setTexture(0, (atomic->geometry->flags & (Geometry::TEXTURED | Geometry::TEXTURED2)) ? mat->texture : nil);
		drawInst_simple(header, inst);
	}
	teardownVertexInput(header);
}

void
RenderDirectionalShadowMap(void)
{
	using namespace rw;
	using namespace rw::gl3;
	using namespace DirectionalShadowPlan;
	using namespace ModernRenderDiagnostics;
	directionalShadowValid = false;
	if(!CModernConfig::m_bMasterEnable || CModernConfig::m_nRealtimeShadows == 0){
		Report(Shadow, Disabled, 0, 0, 100, CModernConfig::m_nRealtimeShadows);
		directionalShadowHasBasis = false;
		return;
	}
	if(!modernWorldShader || !directionalShadowShader){
		Report(Shadow, ShaderUnavailable, 0, 0, 100, !modernWorldShader ? 1 : 2);
		directionalShadowHasBasis = false;
		return;
	}
	if(CGame::IsInInterior() || CCutsceneMgr::IsCutsceneProcessing()){
		Report(Shadow, SceneExcluded);
		directionalShadowHasBasis = false;
		return;
	}
	if(!Scene.camera){ Report(Shadow, CameraUnavailable); directionalShadowHasBasis = false; return; }
	Camera *camera = Scene.camera;
	if(camera->projection != Camera::PERSPECTIVE || !camera->getFrame() ||
	   camera->viewOffset.x != 0.0f || camera->viewOffset.y != 0.0f){
		Report(Shadow, InvalidView);
		return;
	}
	const Matrix *frame = camera->getFrame()->getLTM();
	Vec3 eye = {frame->pos.x, frame->pos.y, frame->pos.z};
	Vec3 right = {frame->right.x, frame->right.y, frame->right.z};
	Vec3 up = {frame->up.x, frame->up.y, frame->up.z};
	Vec3 forward = {frame->at.x, frame->at.y, frame->at.z};
	CVector &sun = CTimeCycle::GetSunDirection();
	CelestialLight light;
	if(!SelectCelestialLight({sun.x, sun.y, sun.z}, &light) || light.strength <= 0.0){
		Report(Shadow, NoCelestialLight);
		directionalShadowHasBasis = false;
		return;
	}
	Vec3 corners[8];
	float maxDistance = CModernConfig::m_nRealtimeShadows >= 2 ? 100.0f : 70.0f;
	float farDistance = Min(camera->farPlane, maxDistance);
	if(!PerspectiveCorners(eye, right, up, forward, camera->viewWindow.x, camera->viewWindow.y,
	                       camera->nearPlane, farDistance, corners)){
		Report(Shadow, InvalidView);
		return;
	}
	int size = CModernConfig::m_nRealtimeShadows >= 2 ? 1024 : 512;
	if(!CreateDirectionalShadowResources(size)){ Report(Shadow, AllocationFailed, size, size); return; }
	if(!DirectionalShadowFit::Build(corners, light.direction, size, 3.0, 45.0, &directionalShadowFit,
	          directionalShadowHasBasis ? &directionalShadowRight : nullptr)){
		Report(Shadow, FitFailed, size, size);
		return;
	}
	directionalShadowRight = directionalShadowFit.right;
	directionalShadowHasBasis = true;
	for(int row = 0; row < 4; ++row)
		for(int col = 0; col < 4; ++col)
			directionalShadowMatrix[col*4+row] = (float)directionalShadowFit.worldToClip[row*4+col];
	directionalShadowStrength = (float)(0.38 * light.strength);

	ShadowCandidates candidates;
	candidates.eye = eye;
	CRenderer::ForEachShadowCandidate(CollectShadowCandidate, &candidates);
	if(candidates.collector.count == 0){ Report(Shadow, NoCasters, size, size); return; }

	Camera *previousCamera = rw::engine->currentCamera;
	World *previousWorld = rw::engine->currentWorld;
	RwCameraBeginUpdate(directionalShadowCamera);
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
		Report(Shadow, TargetFailed, size, size);
		RwCameraEndUpdate(directionalShadowCamera);
		rw::engine->currentCamera = previousCamera;
		rw::engine->currentWorld = previousWorld;
		DestroyDirectionalShadowResources();
		directionalShadowFailedSize = size;
		return;
	}
	RGBA clear = {255,255,255,255};
	RwCameraClear(directionalShadowCamera, &clear, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
	const int states[] = {VERTEXALPHA, ZTESTENABLE, ZWRITEENABLE, CULLMODE, STENCILENABLE,
	                      SRCBLEND, DESTBLEND, TEXTUREADDRESSU, TEXTUREADDRESSV, TEXTUREFILTER};
	uint32 oldStates[sizeof(states)/sizeof(states[0])];
	for(unsigned i = 0; i < sizeof(states)/sizeof(states[0]); ++i) oldStates[i] = GetRenderState(states[i]);
	void *oldRaster = GetRenderStatePtr(TEXTURERASTER);
	SetRenderState(VERTEXALPHA, 0);
	SetRenderState(ZTESTENABLE, 1);
	SetRenderState(ZWRITEENABLE, 1);
	SetRenderState(CULLMODE, CULLNONE);
	SetRenderState(STENCILENABLE, 0);
	// Texture alpha can turn blending on even with VERTEXALPHA disabled.
	SetRenderState(SRCBLEND, BLENDONE);
	SetRenderState(DESTBLEND, BLENDZERO);
	GLboolean dither = glIsEnabled(GL_DITHER);
	if(dither) glDisable(GL_DITHER);
	directionalShadowShader->use();
	glUniformMatrix4fv(U(u_shadowMatrix), 1, GL_FALSE, directionalShadowMatrix);
	for(std::size_t i = 0; i < candidates.collector.count; ++i){
		CEntity *entity = reinterpret_cast<CEntity*>(candidates.collector.entries[i].id);
		DrawShadowAtomic((Atomic*)entity->m_rwObject);
	}
	if(dither) glEnable(GL_DITHER);
	SetRenderStatePtr(TEXTURERASTER, oldRaster);
	for(unsigned i = 0; i < sizeof(states)/sizeof(states[0]); ++i) SetRenderState(states[i], oldStates[i]);
	RwCameraEndUpdate(directionalShadowCamera);
	rw::engine->currentCamera = previousCamera;
	rw::engine->currentWorld = previousWorld;
	directionalShadowValid = true;
	Report(Shadow, Submitted, size, size, 100, int(candidates.collector.count));
}

static void
UploadDirectionalShadowState(bool active)
{
	using namespace rw::gl3;
	float params[4] = { directionalShadowStrength,
	                    active ? 1.0f/directionalShadowSize : 0.0f,
	                    0.0015f, active ? 1.0f : 0.0f };
	glUniformMatrix4fv(U(u_shadowMatrix), 1, GL_FALSE, directionalShadowMatrix);
	glUniform4fv(U(u_shadowParams), 1, params);
	setTexture(3, active ? directionalShadowTexture : nil);
}
#endif

/*
 * Neo Vehicle pipe
 */

rw::gl3::Shader *neoVehicleShader;

static void
uploadSpecLights(void)
{
	using namespace rw::gl3;

	rw::RGBAf colors[1 + NUMEXTRADIRECTIONALS];
	struct {
		rw::V3d dir;
		float power;
	} dirs[1 + NUMEXTRADIRECTIONALS];
	memset(colors, 0, sizeof(colors));
	memset(dirs, 0, sizeof(dirs));
	for(int i = 0; i < 1+NUMEXTRADIRECTIONALS; i++)
		dirs[i].power = 1.0f;
	float power = Power.Get();
	Color speccol = SpecColor.Get();
	colors[0].red = speccol.r;
	colors[0].green = speccol.g;
	colors[0].blue = speccol.b;
	dirs[0].dir = pDirect->getFrame()->getLTM()->at;
	dirs[0].power = power;
	for(int i = 0; i < NUMEXTRADIRECTIONALS; i++){
		if(pExtraDirectionals[i]->getFlags() & rw::Light::LIGHTATOMICS){
			colors[1+i] = pExtraDirectionals[i]->color;
			dirs[1+i].dir = pExtraDirectionals[i]->getFrame()->getLTM()->at;
			dirs[1+i].power = power*2.0f;
		}
	}
	glUniform4fv(U(u_specDir), 1 + NUMEXTRADIRECTIONALS, (float*)&dirs);
	glUniform4fv(U(u_specColor), 1 + NUMEXTRADIRECTIONALS, (float*)&colors);
}

static void
vehicleRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

	// TODO: make this less of a kludge
	if(VehiclePipeSwitch == VEHICLEPIPE_MATFX){
		matFXGlobals.pipelines[rw::platform]->render(atomic);
		return;
	}

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

#ifdef MODERN_FEATURES
	if(UseModernLights(atomic, modernVehicleShader)){
		modernVehicleShader->use();
		UploadModernLights(atomic);
	}else
#endif
		neoVehicleShader->use();

	V3d eyePos = rw::engine->currentCamera->getFrame()->getLTM()->pos;
	glUniform3fv(U(u_eye), 1, (float*)&eyePos);

	uploadSpecLights();

	float reflProps[4];
	reflProps[0] = Fresnel.Get();
	reflProps[1] = SpecColor.Get().a;

	setTexture(1, EnvMapTex);

	SetRenderState(SRCBLEND, BLENDONE);

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		reflProps[2] = m->surfaceProps.specular * VehicleShininess;
		reflProps[3] = m->surfaceProps.specular == 0.0f ? 0.0f : VehicleSpecularity;
		glUniform4fv(U(u_reflProps), 1, reflProps);

		drawInst(header, inst);
		inst++;
	}

	setTexture(1, nil);

	SetRenderState(SRCBLEND, BLENDSRCALPHA);

	teardownVertexInput(header);
}

void
CreateVehiclePipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	if(CFileMgr::LoadFile("neo/carTweakingTable.dat", work_buff, sizeof(work_buff), "r") <= 0)
		printf("Error: couldn't open 'neo/carTweakingTable.dat'\n");
	else{
		char *fp = (char*)work_buff;
		fp = ReadTweakValueTable(fp, Fresnel);
		fp = ReadTweakValueTable(fp, Power);
		fp = ReadTweakValueTable(fp, DiffColor);
		fp = ReadTweakValueTable(fp, SpecColor);
	}


	{
#include "shaders/obj/neoVehicle_frag.inc"
#include "shaders/obj/neoVehicle_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoVehicle_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoVehicle_frag_src, nil };
	neoVehicleShader = Shader::create(vs, fs);
	assert(neoVehicleShader);
	}
#ifdef MODERN_FEATURES
	{
#include "shaders/obj/modernVehicle_vert.inc"
#include "shaders/obj/modernLocal_frag.inc"
#include "shaders/obj/modernVehicle_frag.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, modernVehicle_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernLocal_frag_src, modernVehicle_frag_src, nil };
	modernVehicleShader = Shader::create(vs, fs);
	}
#endif

	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = vehicleRenderCB;
	vehiclePipe = pipe;
}

void
DestroyVehiclePipe(void)
{
#ifdef MODERN_FEATURES
	if(modernVehicleShader){ modernVehicleShader->destroy(); modernVehicleShader = nil; }
#endif
	neoVehicleShader->destroy();
	neoVehicleShader = nil;

	((rw::gl3::ObjPipeline*)vehiclePipe)->destroy();
	vehiclePipe = nil;
}



/*
 * Neo World pipe
 */

rw::gl3::Shader *neoWorldShader;

static void
worldRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

#ifdef MODERN_FEATURES
#if defined(RW_GL3)
	bool shadowActive = directionalShadowValid && !bRenderingEnvMap;
#else
	const bool shadowActive = false;
#endif
	bool useModern = modernWorldShader && CModernConfig::m_bMasterEnable && !bRenderingEnvMap &&
	                 (shadowActive || CModernConfig::m_bDynamicPointLights ||
	                  (modernRenderingRoad && CModernConfig::m_bWetRoadReflections && CWeather::WetRoads > 0.0f));
#else
	const bool useModern = false;
#endif
	if(!LightmapEnable && !useModern){
		gl3::defaultRenderCB(atomic, header);
		return;
	}

	Material *m;

	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

#ifdef MODERN_FEATURES
	if(useModern){
		ModernRenderDiagnostics::Report(ModernRenderDiagnostics::WorldGeometry, ModernRenderDiagnostics::Selected,
			0, 0, 100, ((atomic->geometry->flags & Geometry::NORMALS) ? 1 : 0) |
			(modernRenderingRoad ? 2 : 0) | (shadowActive ? 4 : 0));
		modernWorldShader->use();
		ModernLightUniforms lights;
		BuildModernLights(atomic, CModernConfig::m_bDynamicPointLights != 0, lights);
		glUniform4fv(U(u_modernPosition), ModernLightingPolicy::MaxLights, &lights.position[0][0]);
		glUniform4fv(U(u_modernColor), ModernLightingPolicy::MaxLights, &lights.color[0][0]);
		glUniform4fv(U(u_modernDirection), ModernLightingPolicy::MaxLights, &lights.direction[0][0]);
		V3d eye = rw::engine->currentCamera->getFrame()->getLTM()->pos;
		glUniform3fv(U(u_eye), 1, (float*)&eye);
		bool envAvailable = EnvMapTex && EnvMapHasFrame && VehiclePipeSwitch == VEHICLEPIPE_NEO;
		float options[4] = { modernRenderingRoad && CModernConfig::m_bWetRoadReflections ?
		                     Min(Max(CWeather::WetRoads, 0.0f), 1.0f) : 0.0f,
		                     envAvailable ? 1.0f : 0.0f,
		                     (atomic->geometry->flags & Geometry::NORMALS) ? 1.0f : 0.0f, 0.0f };
		float sky[4] = { CTimeCycle::GetSkyBottomRed()/255.0f,
		                 CTimeCycle::GetSkyBottomGreen()/255.0f,
		                 CTimeCycle::GetSkyBottomBlue()/255.0f, 0.0f };
		glUniform4fv(U(u_modernOptions), 1, options);
		glUniform4fv(U(u_modernSky), 1, sky);
		setTexture(2, envAvailable ? EnvMapTex : nil);
#if defined(RW_GL3)
		UploadDirectionalShadowState(shadowActive);
#endif
	}else
#endif
		neoWorldShader->use();

	float lightfactor[4];

	while(n--){
		m = inst->material;

		if(LightmapEnable && MatFX::getEffects(m) == MatFX::DUAL){
			MatFX *matfx = MatFX::get(m);
			Texture *dualtex = matfx->getDualTexture();
			if(dualtex == nil)
				goto notex;
			setTexture(1, dualtex);
			lightfactor[0] = lightfactor[1] = lightfactor[2] = WorldLightmapBlend.Get()*LightmapMult;
		}else{
		notex:
			setTexture(1, nil);
			lightfactor[0] = lightfactor[1] = lightfactor[2] = 0.0f;
		}
		lightfactor[3] = useModern && !LightmapEnable ? 1.0f : m->color.alpha/255.0f;
		glUniform4fv(U(u_lightMap), 1, lightfactor);

		RGBA color = { 255, 255, 255, m->color.alpha };
		if(useModern && !LightmapEnable)
			setMaterial(atomic->geometry->flags, m->color, m->surfaceProps);
		else
			setMaterial(color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		drawInst(header, inst);
		inst++;
	}
	setTexture(1, nil);
#ifdef MODERN_FEATURES
	if(useModern){
		setTexture(2, nil);
#if defined(RW_GL3)
		setTexture(3, nil);
#endif
	}
#endif
	teardownVertexInput(header);
}

void
CreateWorldPipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	if(CFileMgr::LoadFile("neo/worldTweakingTable.dat", work_buff, sizeof(work_buff), "r") <= 0)
		printf("Error: couldn't open 'neo/worldTweakingTable.dat'\n");
	else
		ReadTweakValueTable((char*)work_buff, WorldLightmapBlend);

	{
#include "shaders/obj/neoWorldVC_frag.inc"
#include "shaders/obj/default_UV2_vert.inc"
	const char *vs[] = { shaderDecl, header_vert_src, default_UV2_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoWorldVC_frag_src, nil };
	neoWorldShader = Shader::create(vs, fs);
	assert(neoWorldShader);
	}
#ifdef MODERN_FEATURES
	{
#include "shaders/obj/modernWorld_vert.inc"
#include "shaders/obj/modernWorld_frag.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n", header_vert_src, modernWorld_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernWorld_frag_src, nil };
	modernWorldShader = Shader::create(vs, fs); // nil = classic world fallback
	}
#if defined(RW_GL3)
	{
#include "shaders/obj/directionalShadow_vert.inc"
#include "shaders/obj/directionalShadow_frag.inc"
	const char *vs[] = { shaderDecl, header_vert_src, directionalShadow_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, directionalShadow_frag_src, nil };
	directionalShadowShader = Shader::create(vs, fs);
	}
#endif
#endif

	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = worldRenderCB;
	worldPipe = pipe;
}

void
DestroyWorldPipe(void)
{
#ifdef MODERN_FEATURES
#if defined(RW_GL3)
	DestroyDirectionalShadowResources();
	directionalShadowFailedSize = 0;
	if(directionalShadowShader){ directionalShadowShader->destroy(); directionalShadowShader = nil; }
#endif
	if(modernWorldShader){ modernWorldShader->destroy(); modernWorldShader = nil; }
#endif
	neoWorldShader->destroy();
	neoWorldShader = nil;

	((rw::gl3::ObjPipeline*)worldPipe)->destroy();
	worldPipe = nil;
}




/*
 * Neo Gloss pipe
 */

rw::gl3::Shader *neoGlossShader;

static void
glossRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

#ifdef MODERN_FEATURES
	modernRenderingRoad = true;
#endif
	worldRenderCB(atomic, header);
#ifdef MODERN_FEATURES
	modernRenderingRoad = false;
#endif
	if(!GlossEnable)
		return;

	Material *m;

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

	neoGlossShader->use();

	V3d eyePos = rw::engine->currentCamera->getFrame()->getLTM()->pos;
	glUniform3fv(U(u_eye), 1, (float*)&eyePos);
	float reflProps[4];
	reflProps[0] = GlossMult;
	reflProps[1] = 0.0f;
	reflProps[2] = 0.0f;
	reflProps[3] = 0.0f;
	glUniform4fv(U(u_reflProps), 1, reflProps);

	SetRenderState(VERTEXALPHA, TRUE);
	SetRenderState(SRCBLEND, BLENDONE);
	SetRenderState(DESTBLEND, BLENDONE);
	SetRenderState(ZWRITEENABLE, FALSE);
	SetRenderState(ALPHATESTFUNC, ALPHAALWAYS);

	while(n--){
		m = inst->material;

		RGBA color = { 255, 255, 255, m->color.alpha };
		setMaterial(color, m->surfaceProps);

		if(m->texture){
			Texture *tex = GetGlossTex(m);
			if(tex){
				setTexture(0, tex);
				drawInst(header, inst);
			}
		}
		inst++;
	}

	SetRenderState(ZWRITEENABLE, TRUE);
	SetRenderState(ALPHATESTFUNC, ALPHAGREATEREQUAL);
	SetRenderState(SRCBLEND, BLENDSRCALPHA);
	SetRenderState(DESTBLEND, BLENDINVSRCALPHA);

	teardownVertexInput(header);
}

void
CreateGlossPipe(void)
{
	using namespace rw;
	using namespace rw::gl3;

	{
#include "shaders/obj/neoGloss_frag.inc"
#include "shaders/obj/neoGloss_vert.inc"
	const char *vs[] = { shaderDecl, header_vert_src, neoGloss_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, neoGloss_frag_src, nil };
	neoGlossShader = Shader::create(vs, fs);
	assert(neoGlossShader);
	}

	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = glossRenderCB;
	glossPipe = pipe;
}

void
DestroyGlossPipe(void)
{
	neoGlossShader->destroy();
	neoGlossShader = nil;

	((rw::gl3::ObjPipeline*)glossPipe)->destroy();
	glossPipe = nil;
}



/*
 * Neo Rim pipes
 */

rw::gl3::Shader *neoRimShader;
rw::gl3::Shader *neoRimSkinShader;

static void
uploadRimData(bool enable)
{
	using namespace rw;
	using namespace rw::gl3;

	V3d viewVec = rw::engine->currentCamera->getFrame()->getLTM()->at;
	glUniform3fv(U(u_viewVec), 1, (float*)&viewVec);
	float rimData[4];
	rimData[0] = Offset.Get();
	rimData[1] = Scale.Get();
	if(enable)
		rimData[2] = Scaling.Get()*RimlightMult;
	else
		rimData[2] = 0.0f;
	rimData[3] = 0.0f;
	glUniform3fv(U(u_rimData), 1, rimData);
	Color col = RampStart.Get();
	glUniform4fv(U(u_rampStart), 1, (float*)&col);
	col = RampEnd.Get();
	glUniform4fv(U(u_rampEnd), 1, (float*)&col);
}

static void
rimSkinRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

#ifdef MODERN_FEATURES
	bool useModern = UseModernLights(atomic, modernPedSkinShader);
#else
	const bool useModern = false;
#endif
	if(!RimlightEnable && !useModern){
		gl3::skinRenderCB(atomic, header);
		return;
	}

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

#ifdef MODERN_FEATURES
	if(useModern){
		modernPedSkinShader->use();
		UploadModernLights(atomic);
	}else
#endif
		neoRimSkinShader->use();

	uploadRimData(RimlightEnable && (atomic->geometry->flags & Geometry::LIGHT));

	uploadSkinMatrices(atomic);

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		drawInst(header, inst);
		inst++;
	}
	teardownVertexInput(header);
}

static void
rimRenderCB(rw::Atomic *atomic, rw::gl3::InstanceDataHeader *header)
{
	using namespace rw;
	using namespace rw::gl3;

#ifdef MODERN_FEATURES
	bool useModern = UseModernLights(atomic, modernPedShader);
#else
	const bool useModern = false;
#endif
	if(!RimlightEnable && !useModern){
		gl3::defaultRenderCB(atomic, header);
		return;
	}

	Material *m;

	rw::uint32 flags = atomic->geometry->flags;
	setWorldMatrix(atomic->getFrame()->getLTM());
	lightingCB(atomic);

	setupVertexInput(header);

	InstanceData *inst = header->inst;
	rw::int32 n = header->numMeshes;

#ifdef MODERN_FEATURES
	if(useModern){
		modernPedShader->use();
		UploadModernLights(atomic);
	}else
#endif
		neoRimShader->use();

	uploadRimData(RimlightEnable && (atomic->geometry->flags & Geometry::LIGHT));

	while(n--){
		m = inst->material;

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		rw::SetRenderState(VERTEXALPHA, inst->vertexAlpha || m->color.alpha != 0xFF);

		drawInst(header, inst);
		inst++;
	}
	teardownVertexInput(header);
}

void
CreateRimLightPipes(void)
{
	using namespace rw::gl3;

	if(CFileMgr::LoadFile("neo/rimTweakingTable.dat", work_buff, sizeof(work_buff), "r") <= 0)
		printf("Error: couldn't open 'neo/rimTweakingTable.dat'\n");
	else{
		char *fp = (char*)work_buff;
		fp = ReadTweakValueTable(fp, RampStart);
		fp = ReadTweakValueTable(fp, RampEnd);
		fp = ReadTweakValueTable(fp, Offset);
		fp = ReadTweakValueTable(fp, Scale);
		fp = ReadTweakValueTable(fp, Scaling);
	}

	{
#include "shaders/obj/simple_frag.inc"
#include "shaders/obj/neoRimSkin_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoRimSkin_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, simple_frag_src, nil };
	neoRimSkinShader = Shader::create(vs, fs);
	assert(neoRimSkinShader);
	}

	{
#include "shaders/obj/simple_frag.inc"
#include "shaders/obj/neoRim_vert.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n", header_vert_src, neoRim_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, simple_frag_src, nil };
	neoRimShader = Shader::create(vs, fs);
	assert(neoRimShader);
	}
#ifdef MODERN_FEATURES
	{
#include "shaders/obj/modernPed_vert.inc"
#include "shaders/obj/modernLocal_frag.inc"
#include "shaders/obj/modernPed_frag.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n", header_vert_src, modernPed_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernLocal_frag_src, modernPed_frag_src, nil };
	modernPedShader = Shader::create(vs, fs);
	}
	{
#include "shaders/obj/modernPedSkin_vert.inc"
#include "shaders/obj/modernLocal_frag.inc"
#include "shaders/obj/modernPed_frag.inc"
	const char *vs[] = { shaderDecl, "#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n", header_vert_src, modernPedSkin_vert_src, nil };
	const char *fs[] = { shaderDecl, header_frag_src, modernLocal_frag_src, modernPed_frag_src, nil };
	modernPedSkinShader = Shader::create(vs, fs);
	}
#endif

	rw::gl3::ObjPipeline *pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::defaultInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = rimRenderCB;
	rimPipe = pipe;

	pipe = rw::gl3::ObjPipeline::create();
	pipe->instanceCB = rw::gl3::skinInstanceCB;
	pipe->uninstanceCB = nil;
	pipe->renderCB = rimSkinRenderCB;
	rimSkinPipe = pipe;
}

void
DestroyRimLightPipes(void)
{
#ifdef MODERN_FEATURES
	if(modernPedShader){ modernPedShader->destroy(); modernPedShader = nil; }
	if(modernPedSkinShader){ modernPedSkinShader->destroy(); modernPedSkinShader = nil; }
#endif
	neoRimShader->destroy();
	neoRimShader = nil;

	neoRimSkinShader->destroy();
	neoRimSkinShader = nil;

	((rw::gl3::ObjPipeline*)rimPipe)->destroy();
	rimPipe = nil;

	((rw::gl3::ObjPipeline*)rimSkinPipe)->destroy();
	rimSkinPipe = nil;
}



void
CustomPipeRegisterGL(void)
{
	u_viewVec = rw::gl3::registerUniform("u_viewVec");
	u_rampStart = rw::gl3::registerUniform("u_rampStart");
	u_rampEnd = rw::gl3::registerUniform("u_rampEnd");
	u_rimData = rw::gl3::registerUniform("u_rimData");

	u_lightMap = rw::gl3::registerUniform("u_lightMap");

	u_eye = rw::gl3::registerUniform("u_eye");
	u_reflProps = rw::gl3::registerUniform("u_reflProps");
	u_specDir = rw::gl3::registerUniform("u_specDir");
	u_specColor = rw::gl3::registerUniform("u_specColor");
#ifdef MODERN_FEATURES
	u_modernPosition = rw::gl3::registerUniform("u_modernPosition");
	u_modernColor = rw::gl3::registerUniform("u_modernColor");
	u_modernDirection = rw::gl3::registerUniform("u_modernDirection");
	u_modernOptions = rw::gl3::registerUniform("u_modernOptions");
	u_modernSky = rw::gl3::registerUniform("u_modernSky");
#if defined(RW_GL3)
	u_shadowMatrix = rw::gl3::registerUniform("u_shadowMatrix");
	u_shadowParams = rw::gl3::registerUniform("u_shadowParams");
#endif
#endif
}


}

#endif

#ifdef NEW_RENDERER
#ifndef LIBRW
#error "Need librw for NEW_PIPELINES"
#endif

namespace WorldRender
{

struct BuildingInst
{
	rw::Matrix matrix;
	rw::gl3::InstanceDataHeader *instHeader;
	uint8 fadeAlpha;
	bool lighting;
};
BuildingInst blendInsts[3][2000];
int numBlendInsts[3];

static RwRGBAReal black;

static bool
IsTextureTransparent(RwTexture *tex)
{
	if(tex == nil || tex->raster == nil)
		return false;
	return PLUGINOFFSET(rw::gl3::Gl3Raster, tex->raster, rw::gl3::nativeRasterOffset)->hasAlpha;
}

// Render all opaque meshes and put atomics that needs blending
// into the deferred list.
void
AtomicFirstPass(RpAtomic *atomic, int pass, bool road)
{
	using namespace rw;
	using namespace rw::gl3;

	BuildingInst scratch;
	const bool hasRoom = numBlendInsts[pass] < (int)ARRAY_SIZE(blendInsts[pass]);
	BuildingInst *building = hasRoom ? &blendInsts[pass][numBlendInsts[pass]] : &scratch;

	atomic->getPipeline()->instance(atomic);
	building->instHeader = (gl3::InstanceDataHeader*)atomic->geometry->instData;
	assert(building->instHeader != nil);
	assert(building->instHeader->platform == PLATFORM_GL3);
	building->fadeAlpha = 255;
	building->lighting = !!(atomic->geometry->flags & rw::Geometry::LIGHT);
	rw::uint32 flags = atomic->geometry->flags;

	WorldLights lights;
	lights.numAmbients = 1;
	lights.numDirectionals = 0;
	lights.numLocals = 0;
	if(building->lighting)
		lights.ambient = pAmbient->color;
	else
		lights.ambient = black;

#ifdef MODERN_FEATURES
	const bool useModern = CustomPipes::modernWorldShader &&
		CModernConfig::m_bMasterEnable && !CustomPipes::bRenderingEnvMap &&
		(CustomPipes::directionalShadowValid || CModernConfig::m_bDynamicPointLights ||
		 (road && CModernConfig::m_bWetRoadReflections && CWeather::WetRoads > 0.0f));
#else
	(void)road;
	const bool useModern = false;
#endif
	bool setupDone = false;
	bool defer = false;
	building->matrix = *atomic->getFrame()->getLTM();

	InstanceData *inst = building->instHeader->inst;
	for(rw::uint32 i = 0; i < building->instHeader->numMeshes; i++, inst++){
		Material *m = inst->material;

		if(inst->vertexAlpha || m->color.alpha != 255 ||
		   IsTextureTransparent(m->texture)){
			defer = true;
			continue;
		}

		// alright we're rendering this atomic
		if(!setupDone){
#ifdef MODERN_FEATURES
			if(useModern){
				ModernRenderDiagnostics::Report(ModernRenderDiagnostics::WorldGeometry, ModernRenderDiagnostics::Selected,
					0, 0, 100, ((flags & Geometry::NORMALS) ? 1 : 0) | (road ? 2 : 0) |
					(CustomPipes::directionalShadowValid ? 4 : 0));
				CustomPipes::modernWorldShader->use();
				ModernLightUniforms local;
				BuildModernLights(atomic, CModernConfig::m_bDynamicPointLights != 0, local);
				glUniform4fv(U(CustomPipes::u_modernPosition), ModernLightingPolicy::MaxLights, &local.position[0][0]);
				glUniform4fv(U(CustomPipes::u_modernColor), ModernLightingPolicy::MaxLights, &local.color[0][0]);
				glUniform4fv(U(CustomPipes::u_modernDirection), ModernLightingPolicy::MaxLights, &local.direction[0][0]);
				V3d eye = rw::engine->currentCamera->getFrame()->getLTM()->pos;
				glUniform3fv(U(CustomPipes::u_eye), 1, (float*)&eye);
				bool env = CustomPipes::EnvMapTex && CustomPipes::EnvMapHasFrame && CustomPipes::VehiclePipeSwitch == CustomPipes::VEHICLEPIPE_NEO;
				float options[4] = { road && CModernConfig::m_bWetRoadReflections ? Min(Max(CWeather::WetRoads, 0.0f), 1.0f) : 0.0f,
				                     env ? 1.0f : 0.0f,
				                     (flags & Geometry::NORMALS) ? 1.0f : 0.0f, 0.0f };
				float sky[4] = { CTimeCycle::GetSkyBottomRed()/255.0f, CTimeCycle::GetSkyBottomGreen()/255.0f, CTimeCycle::GetSkyBottomBlue()/255.0f, 0.0f };
				float noLightmap[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
				glUniform4fv(U(CustomPipes::u_modernOptions), 1, options);
				glUniform4fv(U(CustomPipes::u_modernSky), 1, sky);
				glUniform4fv(U(CustomPipes::u_lightMap), 1, noLightmap);
#if defined(RW_GL3) && defined(EXTENDED_PIPELINES)
				CustomPipes::UploadDirectionalShadowState(CustomPipes::directionalShadowValid && !CustomPipes::bRenderingEnvMap);
#endif
				setTexture(1, nil);
				setTexture(2, env ? CustomPipes::EnvMapTex : nil);
			}else
#endif
				defaultShader->use();
			setWorldMatrix(&building->matrix);
			setupVertexInput(building->instHeader);
			setLights(&lights);
			setupDone = true;
		}

		setMaterial(flags, m->color, m->surfaceProps);

		setTexture(0, m->texture);

		drawInst(building->instHeader, inst);
	}
	teardownVertexInput(building->instHeader);
#ifdef MODERN_FEATURES
	if(useModern && setupDone){
		setTexture(2, nil);
#if defined(RW_GL3) && defined(EXTENDED_PIPELINES)
		setTexture(3, nil);
#endif
	}
#endif
	if(defer && hasRoom)
		numBlendInsts[pass]++;
}

void
AtomicFullyTransparent(RpAtomic *atomic, int pass, int fadeAlpha)
{
	using namespace rw;
	using namespace rw::gl3;

	BuildingInst scratch;
	const bool hasRoom = numBlendInsts[pass] < (int)ARRAY_SIZE(blendInsts[pass]);
	BuildingInst *building = hasRoom ? &blendInsts[pass][numBlendInsts[pass]] : &scratch;

	atomic->getPipeline()->instance(atomic);
	building->instHeader = (gl3::InstanceDataHeader*)atomic->geometry->instData;
	assert(building->instHeader != nil);
	assert(building->instHeader->platform == PLATFORM_GL3);
	building->fadeAlpha = fadeAlpha;
	building->lighting = !!(atomic->geometry->flags & rw::Geometry::LIGHT);
	building->matrix = *atomic->getFrame()->getLTM();
	if(hasRoom) numBlendInsts[pass]++;
}

void
RenderBlendPass(int pass)
{
	using namespace rw;
	using namespace rw::gl3;

	defaultShader->use();
	WorldLights lights;
	lights.numAmbients = 1;
	lights.numDirectionals = 0;
	lights.numLocals = 0;

	int i;
	for(i = 0; i < numBlendInsts[pass]; i++){
		BuildingInst *building = &blendInsts[pass][i];

		setupVertexInput(building->instHeader);
		setWorldMatrix(&building->matrix);
		if(building->lighting)
			lights.ambient = pAmbient->color;
		else
			lights.ambient = black;
		setLights(&lights);

		InstanceData *inst = building->instHeader->inst;
		for(rw::uint32 j = 0; j < building->instHeader->numMeshes; j++, inst++){
			Material *m = inst->material;
			if(!inst->vertexAlpha && m->color.alpha == 255 && !IsTextureTransparent(m->texture) && building->fadeAlpha == 255)
				continue;	// already done this one

			rw::RGBA color = m->color;
			color.alpha = (color.alpha * building->fadeAlpha)/255;
			setMaterial(color, m->surfaceProps);	// always modulate here

			setTexture(0, m->texture);

			drawInst(building->instHeader, inst);
		}
		teardownVertexInput(building->instHeader);
	}
}
}
#endif

#endif
