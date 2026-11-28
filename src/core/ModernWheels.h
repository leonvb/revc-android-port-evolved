#ifndef REVC_MODERN_WHEELS_H
#define REVC_MODERN_WHEELS_H
#ifdef MODERN_FEATURES
class CModernWheels
{
public:
	static void Update();
	static void Reset();
	static void Draw();
	static bool InterceptWeapons();
	static bool InterceptRadio();
	static int TakeWeaponTap();
	static bool TakeRadioTap();
	static int TakeRadioSelection();
	static bool IsOpen();
};
#endif
#endif
