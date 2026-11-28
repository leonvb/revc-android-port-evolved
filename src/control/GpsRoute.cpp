#include "common.h"
#include "GpsRoute.h"

#ifdef MODERN_FEATURES
#include "Game.h"
#include "ModernConfig.h"
#include "GpsRouteSearch.h"
#include "GpsGeometry.h"
#include "PathFind.h"
#include "PlayerPed.h"
#include "PlayerInfo.h"
#include "Vehicle.h"
#include "Pools.h"
#include "Radar.h"
#include "Script.h"
#include "Timer.h"
#include "Replay.h"
#include "Font.h"

namespace {
uint16 route[NUM_PATHNODES];
unsigned routeCount;
uint32 lastUpdate;
int32 targetId = -1;
CVector lastStart, lastTarget;
CVector destination;
bool waypoint;
bool valid;

bool SelectTarget(CVector &pos, int32 &id, bool &isWaypoint, bool inVehicle)
{
#ifdef MAP_ENHANCEMENTS
	if (CRadar::TargetMarkerId != -1) {
		pos = CRadar::TargetMarkerPos;
		id = CRadar::TargetMarkerId;
		isWaypoint = true;
		return true;
	}
#endif
	if (!inVehicle || !CTheScripts::IsPlayerOnAMission()) return false;
	int selected = -1;
	CVector target;
	for (int i = 0; i < NUMRADARBLIPS; ++i) {
		const sRadarTrace &b = CRadar::ms_RadarTrace[i];
		if (!b.m_bInUse || b.m_bShortRange || b.m_eRadarSprite != RADAR_SPRITE_NONE ||
		    (b.m_eBlipDisplay != BLIP_DISPLAY_BLIP_ONLY && b.m_eBlipDisplay != BLIP_DISPLAY_BOTH)) continue;
		CEntity *entity = nil;
		if (b.m_eBlipType == BLIP_CAR) entity = CPools::GetVehiclePool()->GetAt(b.m_nEntityHandle);
		else if (b.m_eBlipType == BLIP_CHAR) entity = CPools::GetPedPool()->GetAt(b.m_nEntityHandle);
		else if (b.m_eBlipType != BLIP_COORD) continue;
		if (b.m_eBlipType != BLIP_COORD && !entity) continue;
		if (selected != -1) return false; // no unambiguous objective
		selected = i;
		target = entity ? entity->GetPosition() : b.m_vecPos;
	}
	if (selected < 0) return false;
	pos = target;
	id = int32(uint32(selected) | (uint32(CRadar::ms_RadarTrace[selected].m_BlipIndex) << 16));
	isWaypoint = false;
	return true;
}

void Segment(const CVector2D &a, const CVector2D &b, float width, const CRGBA &colour)
{
	float dx = b.x-a.x, dy = b.y-a.y;
	float length = sqrtf(dx*dx+dy*dy);
	if (length < 0.01f) return;
	float nx = -dy*width/length, ny = dx*width/length;
	CSprite2d::Draw2DPolygon(a.x+nx, a.y+ny, b.x+nx, b.y+ny,
	                         a.x-nx, a.y-ny, b.x-nx, b.y-ny, colour);
}

CVector Position(unsigned i) { return ThePaths.m_pathNodes[route[i]].GetPosition(); }

// Skip route nodes behind the player's projection along the first segment.
CVector FirstPoint(const CVector &player, unsigned &next)
{
	next = 0;
	if (routeCount < 2) return routeCount ? Position(0) : player;
	CVector a = Position(0), b = Position(1);
	float dx = b.x-a.x, dy = b.y-a.y, den = dx*dx+dy*dy;
	float t = den > 0.001f ? ((player.x-a.x)*dx+(player.y-a.y)*dy)/den : 0.0f;
	t = Max(0.0f, Min(1.0f, t));
	next = 1;
	return CVector(a.x+dx*t, a.y+dy*t, a.z+(b.z-a.z)*t);
}
}

void CGpsRoute::Reset()
{
	routeCount = 0;
	valid = false;
	targetId = -1;
	lastUpdate = 0;
}

void CGpsRoute::Update()
{
	CPlayerPed *ped = FindPlayerPed();
	CVehicle *vehicle = FindPlayerVehicle();
	if (!ped || !CModernConfig::m_bMasterEnable || CModernConfig::m_nGpsRouteMode == MODERN_GPS_OFF ||
	    CGame::currArea != AREA_MAIN_MAP || CTheScripts::bPlayerIsInTheStatium || CReplay::IsPlayingBack() ||
	    (vehicle && !vehicle->IsCar() && !vehicle->IsBike()) ||
	    (!vehicle && CModernConfig::m_nGpsRouteMode == MODERN_GPS_IN_VEHICLE)) {
		Reset(); return;
	}
	CVector start = ped->GetPosition(), target;
	int32 id;
	bool newWaypoint;
	if (!SelectTarget(target, id, newWaypoint, vehicle != nil)) { Reset(); return; }
#ifdef MAP_ENHANCEMENTS
	if (newWaypoint && (start-target).Magnitude2D() < 20.0f) {
		CRadar::ClearBlip(CRadar::TargetMarkerId);
		CRadar::TargetMarkerId = -1;
		Reset(); return;
	}
#endif
	uint32 now = CTimer::GetTimeInMilliseconds();
	if (valid && id == targetId && waypoint == newWaypoint && (target-lastTarget).MagnitudeSqr() < 64.0f &&
	    (start-lastStart).MagnitudeSqr() < 64.0f && uint32(now-lastUpdate) < 250) return;
	lastUpdate = now; lastStart = start; lastTarget = target; targetId = id;
	waypoint = newWaypoint; destination = target; valid = true; routeCount = 0;
	int first = CGpsRouteSearch::FindNearest(start);
	if (first < 0) return;
	// Do not silently snap a destination on another island to this component.
	int last = CGpsRouteSearch::FindNearest(target);
	if (last < 0) return;
	routeCount = CGpsRouteSearch::Find(first, last, route, NUM_PATHNODES);
}

void CGpsRoute::Draw(bool menuMap)
{
	if (!valid || !routeCount || !FindPlayerPed()) return;
	CVector player = FindPlayerPed()->GetPosition();
	unsigned next;
	CVector first = FirstPoint(player, next);
	RwUInt32 ztest, zwrite, shade, alpha, srcblend, dstblend;
	RwRaster *raster;
	RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
	RwRenderStateGet(rwRENDERSTATEZWRITEENABLE, &zwrite);
	RwRenderStateGet(rwRENDERSTATESHADEMODE, &shade);
	RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &alpha);
	RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcblend);
	RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dstblend);
	RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &raster);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDSRCALPHA);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDINVSRCALPHA);
	CRGBA colour = waypoint ? CRGBA(224, 86, 253, 255) : CRGBA(251, 197, 49, 255);
	// Inset in radar coordinates by at least the screen-space outline width,
	// including on wide displays where X and Y scales are different.
	float radarRadius = Min(SCREEN_SCALE_X(RADAR_WIDTH) * 0.5f, SCREEN_SCALE_Y(RADAR_HEIGHT) * 0.5f);
	float clipRadius = radarRadius > 0.0f ? 1.0f - SCREEN_SCALE_X(2.6f) / radarRadius - 0.01f : 0.0f;
	// First connect the player to the forward projection, then the remaining road.
	for (unsigned i = 0; i <= routeCount-next+1; ++i) {
		CVector from = i == 0 ? player : (i == 1 ? first : Position(next+i-2));
		CVector to = i == 0 ? first : (i <= routeCount-next ? Position(next+i-1) : destination);
		CVector2D a, b, screenA, screenB;
		CRadar::TransformRealWorldPointToRadarSpace(a, CVector2D(from.x, from.y));
		CRadar::TransformRealWorldPointToRadarSpace(b, CVector2D(to.x, to.y));
		if (!menuMap && !GpsGeometry::ClipCircle(a, b, clipRadius)) continue;
		CRadar::TransformRadarPointToScreenSpace(screenA, a);
		CRadar::TransformRadarPointToScreenSpace(screenB, b);
		if (menuMap && ((screenA.x < -8.0f && screenB.x < -8.0f) ||
		                (screenA.x > SCREEN_WIDTH + 8.0f && screenB.x > SCREEN_WIDTH + 8.0f) ||
		                (screenA.y < -8.0f && screenB.y < -8.0f) ||
		                (screenA.y > SCREEN_HEIGHT + 8.0f && screenB.y > SCREEN_HEIGHT + 8.0f))) continue;
		Segment(screenA, screenB, SCREEN_SCALE_X(2.6f), CRGBA(20, 12, 25, 255));
		Segment(screenA, screenB, SCREEN_SCALE_X(1.5f), colour);
	}
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)(uintptr)ztest);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)(uintptr)zwrite);
	RwRenderStateSet(rwRENDERSTATESHADEMODE, (void*)(uintptr)shade);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)(uintptr)alpha);
	RwRenderStateSet(rwRENDERSTATESRCBLEND, (void*)(uintptr)srcblend);
	RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)(uintptr)dstblend);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, raster);
}

void CGpsRoute::DrawDistance()
{
	if (!valid || !routeCount || !FindPlayerPed()) return;
	CVector player = FindPlayerPed()->GetPosition();
	unsigned next;
	CVector previous = FirstPoint(player, next);
	float distance = (previous-player).Magnitude();
	for (unsigned i = next; i < routeCount; ++i) {
		CVector p = Position(i); distance += (p-previous).Magnitude(); previous = p;
	}
	distance += (destination-previous).Magnitude();
	char text[32]; wchar wide[32];
	snprintf(text, sizeof(text), "%dm", int(distance + 0.5f));
	AsciiToUnicode(text, wide);
	CFont::SetScale(SCREEN_SCALE_X(0.35f), SCREEN_SCALE_Y(0.55f));
	CFont::SetFontStyle(FONT_HEADING);
	CFont::SetCentreOn(); CFont::SetRightJustifyOff(); CFont::SetJustifyOff();
	CFont::SetCentreSize(SCREEN_WIDTH); CFont::SetPropOn();
	CFont::SetBackgroundOff(); CFont::SetDropShadowPosition(1);
	CFont::SetDropColor(CRGBA(0,0,0,255)); CFont::SetColor(CRGBA(255,255,255,255));
	CFont::PrintString(SCREEN_SCALE_X(RADAR_LEFT + RADAR_WIDTH*0.5f), SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM - 6.0f), wide);
	CFont::SetDropShadowPosition(0);
}
#endif
