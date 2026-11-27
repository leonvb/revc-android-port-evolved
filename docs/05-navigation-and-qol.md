# 05 — Navigation, Checkpoints & Quality of Life

## Overview & Goals
Modern open-world games remove tedious friction around navigation, saving, and mission retries without dumbing down gameplay:
1. **Minimap & Pause-Map GPS Route Line**: `reVC` already supports placing a map waypoint (`MAP_ENHANCEMENTS` / `CRadar::TargetMarkerId`) and already holds the entire Vice City road graph in memory (`ThePaths` in `src/control/PathFind.cpp`), yet players still have to pause repeatedly or memorize bridge connections to navigate.
2. **Automatic Post-Mission & Property Autosave**: Forgetting to drive back to Ocean View Hotel or Hyman Condo after finishing a tough mission chain loses progress on a crash, battery drain, or accidental death.
3. **Mid-Mission Checkpoints & Faster Transitions**: `MISSION_REPLAY` is already enabled in `reVC` (allowing instant retry upon mission failure), which can be complemented by optional stage checkpoints and snappier interior/cutscene fades.

---

## Target Files in Codebase

| File | Role |
| :--- | :--- |
| `src/core/config.h` | Add `#define GPS_RADAR_ROUTE`, `#define AUTOSAVE_SYSTEM`, `#define FAST_INTERIOR_FADES` |
| `src/control/PathFind.cpp` & `.h` | Implement `CPathFind::ComputeGpsRoute()` (A* search over `PATH_CAR` road nodes without mutating AI `m_searchNodes` state) |
| `src/core/Radar.cpp` & `.h` | Draw clipped, anti-aliased GPS polyline on both the HUD circular radar (`DrawRadarMap`) and full-screen Pause Menu map (`DrawMap`), plus remaining distance readout |
| `src/save/PCSave.cpp` & `GenericGameStorage.cpp` | Dedicated **Slot 9 (Autosave)** or rotating autosave file (`GTAVCsf9.b` / `autosave.b`) triggered cleanly outside mission scripts |
| `src/control/Script.cpp` & `OnscreenTimer.cpp` | Detect `CTheScripts::IsPlayerOnAMission()` false-transition (mission passed) and property purchase completion to queue safe autosave |
| `src/core/Camera.cpp` & `GameLogic.cpp` | Optional faster interior entrance/exit camera fades (`0.35s` vs `1.0s+`) |

---

## Detailed Design & Implementation Steps

### 1. GTA IV / GTA V-Style Minimap & Map GPS Route (`PathFind.cpp`, `Radar.cpp`)

#### A. Non-Intrusive A* Road Search (`CPathFind::ComputeGpsRoute`)
- **Why a dedicated A* buffer?**
  - `CPathFind::DoPathSearch` mutates `CPathNode::distance` in-place on `ThePaths.m_pathNodes[]` for NPC traffic/police AI. Running GPS in a separate lightweight static array (`g_GpsCost[NUM_PATHNODES]`, `g_GpsParent[NUM_PATHNODES]`, `9650` nodes total = `< 40 KB` RAM!) guarantees **zero interference** with NPC pathfinding and runs in **`< 0.15 ms`** on modern CPUs.
- **Target Selection Priority**:
  1. **User Waypoint (`CRadar::TargetMarkerId != -1`)** — draws a **Purple / Magenta** (`#E056FD`) or classic Vice City **Neon Pink** GPS line, auto-clearing when the player comes within `20 m` of the destination.
  2. **Active Mission Objective Blip** — when no manual waypoint is set and the player is in a land vehicle, automatically select the single active `BLIP_COORD` / `BLIP_CAR` / `BLIP_CHAR` mission marker and draw a **Yellow** (`#FBC531`) GPS line (matching blip color if color-coded).
  3. **Disable Automatically** when the player is in a boat (`CBoat`), helicopter (`CHeli`), plane (`CPlane`), on foot (optional toggle: `GPS: In Vehicles / Always / Off`), or inside an interior (`CGame::currArea != AREA_MAIN_MAP`).
- **Update Cadence & Path Smoothing**:
  - Recompute A* every `250 ms` or whenever the player moves `> 8 m` from the last start node.
  - Trim back-tracking segments behind the vehicle's current bumper projection onto the first road link `(node0 -> node1)` so the line starts cleanly at the player arrow rather than jumping backward to the nearest intersection.
  - Respect `node->bDisabled` so closed bridges early in the story (before *Phnom Penh '86*) aren't routed across unless unlocked!

#### B. Rendering on HUD Minimap & Pause Map (`Radar.cpp`)
- Render in `CRadar::DrawRadarMap()` **after** road map tiles (`DrawRadarSection`) and **before** `DrawBlips()` + player centre arrow so icons always sit cleanly on top of the route line:
  1. Transform each node `CVector2D` from world space → radar space (`TransformRealWorldPointToRadarSpace`) → screen space (`TransformRadarPointToScreenSpace`).
  2. Clip segments to the circular radar mask radius (`LimitRadarPoint` / analytical circle-segment intersection) so the line never bleeds outside the radar ring.
  3. Emit thick triangle-strip quads (`CSprite2d::SetVertices` / `RwIm2DRenderPrimitive`) with a subtle dark border (`1 px` outline) for crisp legibility on bright beach sand and pink map tiles.
  4. Display optional compact distance counter (`e.g., 420m`) beneath the radar.
  5. Also render the full route polyline on the **Frontend Pause Map** (`CRadar::DrawMap()`).

---

### 2. Dedicated Non-Destructive Autosave (`PCSave.cpp`, `Script.cpp`)

#### Safety Rules (Critical in GTA Games)
Saving in Vice City *while* `CTheScripts::IsPlayerOnAMission()` is true or while `TheCamera.m_bWideScreenOn` / fade-ins are active can corrupt script thread IP offsets on reload.

**Safe Autosave Trigger Conditions:**
1. Watch transition of mission completion (`Stats::LastMissionPassedName` updated AND `CTheScripts::IsPlayerOnAMission() == false` AND `FindPlayerPed()->m_nPedState == PED_IDLE / PED_DRIVING` AND player health `> 0` AND `!CReplay::IsPlayingBack()`).
2. Wait a `2.0 second` settle grace period after mission cleanup scripts finish (e.g., post-mission phone call setup).
3. Write to a dedicated **`AUTOSAVE` slot** (`GTAVCsf0.b` or `Slot 9` shown at the top of the Load Game menu) so none of the user's 8 manual save slots (`GTAVCsf1.b`..`GTAVCsf8.b`) are ever overwritten!
4. Show a subtle non-blocking corner indicator (`"Autosaved"` text pulse for `2.5s` in `CHud::Draw`) without pausing gameplay or opening a modal screen.

---

### 3. Mid-Mission Retry Polish & Snappy Fades (`GameLogic.cpp`, `Camera.cpp`)
- **Enhanced `MISSION_REPLAY`**:
  - `reVC` already supports `MISSION_REPLAY`. Extend it with an option to **restore pre-mission armor & ammo** upon retry (instead of penalizing failed attempts by draining all M4/Rocket ammo before restarting the mission).
  - Add an explicit **"Restart Mission"** entry in the Pause Menu while on a mission so the player doesn't have to blow themselves up to retry a botched race or stealth objective.
- **Fast Interior / Garage Fades (`FAST_INTERIOR_FADES`)**:
  - Optional toggle reducing routine interior doorway black-screen fade durations by `50%` while leaving story cutscene dramatic fades untouched.

---

## Verification & Testing Criteria
- **GPS Bridge & One-Way Accuracy**:
  - Test routing across Ocean Beach → Starfish Island → Mainland bridges both **before** and **after** *Phnom Penh '86* bridge unlock (`bDisabled` road nodes).
  - Test routing into multi-level overpasses ( Escobar International Airport highway flyovers ) to verify `z` height separation picks the correct road deck.
- **Autosave Integrity**:
  - Complete first 5 story missions (*In the Beginning*, *An Old Friend*, *The Party*, *Back Alley Brawl*, *Jury Fury*), kill & restart the app, load the Autosave slot, and verify `VALIDATE_SAVE_SIZE` and subsequent mission triggers + cell phone calls fire identically to a manual hotel save.
