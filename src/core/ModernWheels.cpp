#include "common.h"
#ifdef MODERN_FEATURES
#include "ModernWheels.h"
#include "ModernWheelInput.h"
#include "ModernConfig.h"
#include "Camera.h"
#include "CutsceneMgr.h"
#include "Darkel.h"
#include "DMAudio.h"
#include "Frontend.h"
#include "Pad.h"
#include "PlayerPed.h"
#include "Replay.h"
#include "Record.h"
#include "Timer.h"
#include "Vehicle.h"
#include "MusicManager.h"
#include "Radar.h"
#include "Sprite2d.h"
#include "Font.h"
#include "Hud.h"
#include "Text.h"
#include "WeaponInfo.h"
#include "ModelInfo.h"
#include "TxdStore.h"
#include "main.h"

namespace {
ModernWheelInput wheel;
bool initialized = false;
bool interceptWeapon = false, interceptRadio = false;
int weaponTap = 0, radioSelection = -1;
bool radioTap = false;
int kind = 0; // 1 weapon, 2 radio
bool ignoreUntilReleased = false;
bool ownsScale = false, triedScale = false;
float previousScale = 1.0f, lastScale = 1.0f;
uint32 lastClock = 0, scaleRevision = 0;

bool radioButton(CPad *pad)
{
	switch (pad->GetMode()) {
	case 0: return pad->NewState.LeftShoulder1 != 0;
	case 1: return pad->NewState.Select != 0;
	case 2: return pad->NewState.LeftShock != 0;
	case 3: return pad->NewState.Circle != 0;
	default: return false;
	}
}

// Eight groups; a held wheel leaves camera/utility slots available via tap.
int weaponSlot(CPlayerPed *player, int sector)
{
	static const int slots[8] = { WEAPONSLOT_MELEE, WEAPONSLOT_HANDGUN,
		WEAPONSLOT_SHOTGUN, WEAPONSLOT_SUBMACHINEGUN, WEAPONSLOT_RIFLE,
		WEAPONSLOT_HEAVY, WEAPONSLOT_PROJECTILE, WEAPONSLOT_SNIPER };
	if (!player || sector < 0 || sector >= 8) return -1;
	int slot = slots[sector];
	if (slot == WEAPONSLOT_MELEE && !player->HasWeaponSlot(slot)) return WEAPONSLOT_UNARMED;
	if (!player->HasWeaponSlot(slot) || !player->GetWeapon(slot).HasWeaponAmmoToBeUsed()) return -1;
	return slot;
}

void stopScale()
{
	if (ownsScale && CTimer::GetTimeScaleRevision() == scaleRevision)
		CTimer::SetTimeScale(previousScale);
	ownsScale = false;
}

void updateScale(bool open, float dt)
{
	if (ownsScale && CTimer::GetTimeScaleRevision() != scaleRevision)
		ownsScale = false; // A script, adrenaline, replay, etc. took ownership.
	if (open && !triedScale) {
		triedScale = true;
		// Do not take ownership of a script/adrenaline time scale.
		if (Abs(CTimer::GetTimeScale() - 1.0f) < 0.0001f) {
			previousScale = lastScale = CTimer::GetTimeScale();
			ownsScale = true;
			scaleRevision = CTimer::GetTimeScaleRevision();
		}
	}
	if (!ownsScale) return;
	float target = open ? previousScale * 0.25f : previousScale;
	lastScale += (target - lastScale) * (1.0f - expf(-dt * 12.0f));
	if (!open && Abs(lastScale - previousScale) < 0.001f) {
		CTimer::SetTimeScale(previousScale);
		ownsScale = false;
	} else {
		CTimer::SetTimeScale(lastScale);
		scaleRevision = CTimer::GetTimeScaleRevision();
	}
}
}

void CModernWheels::Reset()
{
	stopScale();
	wheel.Reset();
	initialized = true;
	interceptWeapon = interceptRadio = false;
	weaponTap = 0; radioTap = false; radioSelection = -1;
	kind = 0; ignoreUntilReleased = true; triedScale = false; lastClock = 0;
}

void CModernWheels::Update()
{
	if (!initialized) { wheel.Reset(); initialized = true; }
	weaponTap = 0; radioTap = false; radioSelection = -1;
	CPad *pad = CPad::GetPad(0);
	CPlayerPed *player = FindPlayerPed();
	CVehicle *vehicle = FindPlayerVehicle();
	bool valid = CModernConfig::m_bMasterEnable && CModernConfig::m_bWeaponRadioWheels && player &&
		!FrontEndMenuManager.GetIsMenuActive() && !CTimer::GetIsPaused() &&
		!CReplay::IsPlayingBack() && !CRecordDataForGame::IsPlayingBack() &&
		!CCutsceneMgr::IsCutsceneProcessing() && !pad->ArePlayerControlsDisabled() &&
		TheCamera.WhoIsInControlOfTheCamera == CAMCONTROL_GAME && !TheCamera.m_WideScreenOn &&
		!player->DyingOrDead() && !player->m_attachedTo;
	bool onFoot = valid && !player->bInVehicle && player->IsPedInControl() &&
		!player->m_pPointGunAt && !CPlayerPed::bDontAllowWeaponChange &&
		!CDarkel::FrenzyOnGoing() && !TheCamera.Using1stPersonWeaponMode() &&
		player->GetWeapon()->m_eWeaponType != WEAPONTYPE_DETONATOR && !pad->GetTarget();
	bool inCar = valid && vehicle && player->bInVehicle && vehicle->GetStatus() == STATUS_PLAYER &&
		!MusicManager.UsesPoliceRadio(vehicle) && !MusicManager.UsesTaxiRadio(vehicle);
	interceptWeapon = onFoot;
	interceptRadio = inCar;
	uint32 now = CTimer::GetTimeInMillisecondsPauseMode();
	float dt = lastClock ? Min((now - lastClock) * 0.001f, 0.1f) : 0.0f;
	lastClock = now;
	int newKind = onFoot ? 1 : (inCar ? 2 : 0);
	if (!newKind || (kind && kind != newKind)) {
		if (wheel.tracking || wheel.open || ownsScale) {
			wheel.Reset(); stopScale(); triedScale = false; ignoreUntilReleased = true;
		}
		kind = newKind;
		return;
	}
	kind = newKind;
	bool left = kind == 1 && pad->GetLeftShoulder2();
	bool right = kind == 1 ? pad->GetRightShoulder2() : radioButton(pad);
	if (ignoreUntilReleased) {
		if (!left && !right) ignoreUntilReleased = false;
		updateScale(false, dt);
		return;
	}
	bool wasOpen = wheel.open;
	ModernWheelInput::Event event = wheel.Update(now, left, right,
		(float)pad->GetRightStickX(), (float)pad->GetRightStickY(), kind == 1 ? 8 : 10);
	if (!wasOpen && wheel.open) triedScale = false;
	if (event == ModernWheelInput::TAP) {
		if (kind == 1) weaponTap = wheel.button;
		else radioTap = true;
	} else if (event == ModernWheelInput::SELECT) {
		if (kind == 1) {
			int slot = weaponSlot(player, wheel.selected);
			if (slot >= 0) player->m_nSelectedWepSlot = slot;
		} else if (wheel.selected >= 0) {
			// Nine standard stations plus off, intentionally excluding user MP3s.
			radioSelection = wheel.selected == 9 ? RADIO_OFF : wheel.selected;
		}
		ignoreUntilReleased = left || right;
	}
	updateScale(wheel.open, dt);
}

bool CModernWheels::InterceptWeapons() { return interceptWeapon; }
bool CModernWheels::InterceptRadio() { return interceptRadio; }
int CModernWheels::TakeWeaponTap() { int v = weaponTap; weaponTap = 0; return v; }
bool CModernWheels::TakeRadioTap() { bool v = radioTap; radioTap = false; return v; }
int CModernWheels::TakeRadioSelection() { int v = radioSelection; radioSelection = -1; return v; }
bool CModernWheels::IsOpen() { return wheel.open; }

void CModernWheels::Draw()
{
	if (!wheel.open || !kind) return;
	CPlayerPed *player = FindPlayerPed();
	if (!player) return;
	const int count = kind == 1 ? 8 : 10;
	const float cx = SCREEN_WIDTH * 0.5f, cy = SCREEN_HEIGHT * 0.5f;
	const float rx = SCREEN_SCALE_X(135.0f), ry = SCREEN_SCALE_Y(105.0f);
	for (int i = 0; i < count; ++i) {
		float a = TWOPI * i / count;
		float x = cx + Sin(a)*rx, y = cy - Cos(a)*ry;
		float w = SCREEN_SCALE_X(21.0f), h = SCREEN_SCALE_Y(21.0f);
		bool selected = i == wheel.selected;
		CSprite2d::DrawRect(CRect(x-w, y-h, x+w, y+h), selected ? CRGBA(220, 145, 70, 220) : CRGBA(0, 0, 0, 170));
		if (kind == 2 && i < 9) {
			CRadar::DrawRadarSprite(RADAR_SPRITE_RADIO_WILDSTYLE + i, x, y, 255);
		} else if (kind == 1) {
			int slot = weaponSlot(player, i);
			if (slot >= 0) {
				CWeaponInfo *info = CWeaponInfo::GetWeaponInfo(player->GetWeapon(slot).m_eWeaponType);
				CBaseModelInfo *model = info->m_nModelId > 0 ? CModelInfo::GetModelInfo(info->m_nModelId) : nil;
				TxdDef *txd = model && model->GetTxdSlot() >= 0 ? CTxdStore::GetSlot(model->GetTxdSlot()) : nil;
				if (slot == WEAPONSLOT_UNARMED) {
					CHud::Sprites[HUD_FIST].Draw(CRect(x-w, y-h, x+w, y+h), CRGBA(255,255,255,255));
				} else if (txd && txd->texDict) {
					RwTexture *texture = RwTexDictionaryFindNamedTexture(txd->texDict, model->GetModelName());
					if (texture) {
						CSprite2d sprite; sprite.m_pTexture = texture;
						sprite.Draw(CRect(x-w, y-h, x+w, y+h), CRGBA(255,255,255,255));
						sprite.m_pTexture = nil;
					}
				}
			}
		}
	}
	char label[40];
	if (kind == 2) {
		static const char *names[] = { "WILDSTYLE", "FLASH FM", "KCHAT", "FEVER", "V-ROCK", "VCPR", "ESPANTOSO", "EMOTION", "WAVE", "OFF" };
		snprintf(label, sizeof(label), "%s", wheel.selected >= 0 ? names[wheel.selected] : "RADIO");
	} else {
		int slot = weaponSlot(player, wheel.selected);
		if (slot >= 0) snprintf(label, sizeof(label), "AMMO %d", player->GetWeapon(slot).m_nAmmoTotal);
		else snprintf(label, sizeof(label), "%s", wheel.selected < 0 ? "WEAPONS" : "EMPTY");
	}
	wchar wide[40]; AsciiToUnicode(label, wide);
	CFont::SetBackgroundOff(); CFont::SetCentreOn(); CFont::SetCentreSize(SCREEN_WIDTH);
	CFont::SetScale(SCREEN_SCALE_X(0.45f), SCREEN_SCALE_Y(0.7f));
	CFont::SetFontStyle(FONT_STANDARD); CFont::SetColor(CRGBA(255,255,255,255));
	CFont::SetDropShadowPosition(0); CFont::PrintString(cx, cy, wide);
}
#endif
