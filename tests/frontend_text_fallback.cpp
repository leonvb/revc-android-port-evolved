// Exercise the real CText lookup with tiny synthetic GXT tables.
#include "common.h"
#ifdef TEST_WITHOUT_MODERN_FEATURES
#undef MODERN_FEATURES
#endif
// Expose CText's tables in this test only, to avoid depending on game assets.
#define class struct
#include "Text.h"
#undef class
#include "../src/text/Text.cpp"
#undef main
#include <cassert>
#include <string>

static std::string ascii(const wchar *text)
{
	std::string result;
	while (*text) result += static_cast<char>(*text++);
	return result;
}

static void setEntry(CKeyArray &keys, CData &data, const char *key, const char *value)
{
	keys.entries = new CKeyEntry[1]();
	keys.numEntries = 1;
	strcpy(keys.entries[0].key, key);
	keys.entries[0].valueOffset = 0;
	data.numChars = strlen(value) + 1;
	data.chars = new wchar[data.numChars];
	for (int i = 0; i < data.numChars; ++i) data.chars[i] = value[i];
}

int main()
{
	CText text;
	assert(ascii(text.Get("FET_GFX")) == "GRAPHICS SETUP");
	assert(ascii(text.Get("FED_CLF")) == "COLOUR FILTER");
	assert(ascii(text.Get("FEM_NON")) == "NONE");
	assert(ascii(text.Get("FEM_CAR")) == "CAR DENSITY");
	assert(ascii(text.Get("FEC_JPR")).size() > 40);
#ifdef MODERN_FEATURES
	assert(ascii(text.Get("FEM_TFP")) == "TARGET FPS");
	assert(ascii(text.Get("FFR_RAT")) == "FRAME RATE");
	assert(ascii(text.Get("FFR_UNC")) == "UNCAPPED");
#endif
#ifndef MASTER
	assert(ascii(text.Get("UNKNOWN")) == "UNKNOWN missing");
#endif
	wchar *first = text.Get("FED_CLF");
	wchar *second = text.Get("FEM_NON");
	assert(ascii(first) == "COLOUR FILTER" && ascii(second) == "NONE");

	// Translations in the loaded main or mission table must win over fallback.
	setEntry(text.keyArray, text.data, "FED_CLF", "FILTRE");
	setEntry(text.mission_keyArray, text.mission_data, "FET_GFX", "MISSION GRAPHICS");
	text.bHasMissionTextOffsets = text.bIsMissionTextLoaded = true;
	assert(ascii(text.Get("FED_CLF")) == "FILTRE");
	assert(ascii(text.Get("FET_GFX")) == "MISSION GRAPHICS");
	assert(ascii(text.Get("FEM_CAR")) == "CAR DENSITY");
}
