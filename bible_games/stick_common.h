// stick_common.h - pieces shared by the stick's screens: the game selector,
// Bible Trivia (landscape) and Tower of Babel (portrait).
#pragma once
#include <M5Unified.h>
#include <Preferences.h>
#include <TowerOfBabel.h>      // lib/TowerOfBabel

// ---- Idle / power settings -------------------------------------------------
constexpr uint32_t DIM_AFTER_MS   = 30000;    // dim screen after 30 s idle
constexpr uint32_t OFF_AFTER_MS   = 180000;   // power off after 3 min idle (battery only)
constexpr uint8_t  DIM_BRIGHTNESS = 16;       // 0-255

extern uint8_t     normalBrightness;
extern Preferences prefs;          // namespace "trivia": sound, towerBest
extern Framebuffer g_fb;           // 135x240 portrait screen buffer

bool onUsbPower();
void powerOffNow();                // returns only if power-off failed
void pushFramebuffer();            // g_fb -> display (portrait)

// For screens that poll the buttons themselves. Call idleUpdate() every
// loop with that loop's presses: a press that only wakes a dimmed screen is
// cleared (a and b become false), and the stick dims / powers off when idle.
struct IdleTimer {
    uint32_t lastInput = 0;
    bool     dimmed = false;
};
void idleStart(IdleTimer& t);
void idleUpdate(IdleTimer& t, bool& a, bool& b);

// ---- Screens -----------------------------------------------------------------
enum class GameChoice { Trivia, Tower };
GameChoice runSelector();   // handles the Sound and Power off cards itself
void runTrivia();           // returns when the player picks "Back to games"
void runTower();            // returns when the player presses Side on its title
void runTowerBenchmark();   // diagnostic, triggered over serial
