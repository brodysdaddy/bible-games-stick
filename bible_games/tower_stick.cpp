// tower_stick.cpp - runs Tower of Babel on the stick (portrait, 135x240).
//
// The game itself (rules, visuals, sound data) comes from the TowerOfBabel
// library, the same code as the Windows prototype. This file only supplies
// what the stick provides: buttons, a 30 fps loop, the display, the speaker,
// and flash storage for the high score.
//   Front button (BtnA) : start / drop / play again
//   Side button  (BtnB) : pause / resume / menu; on the title, back to games
#include "stick_common.h"
#include "sound_player.h"   // isSoundEnabled(): one sound setting for both games
#include <esp_random.h>

Framebuffer g_fb;

void pushFramebuffer() {
    // rgb565_t = plain RGB565, which is what Framebuffer stores.
    M5.Display.pushImage(0, 0, Framebuffer::W, Framebuffer::H,
                         (const lgfx::rgb565_t*)g_fb.pixels);
}

// ---- Non-blocking sound: a tiny sequencer polled from the game loop -------
namespace {
const int MAX_TONES = 16;
Tone     g_seq[MAX_TONES];
uint8_t  g_seqCount = 0, g_seqIndex = 0;
uint32_t g_seqNext = 0;

void soundTick() {
    if (g_seqIndex >= g_seqCount) return;
    uint32_t now = millis();
    if ((int32_t)(now - g_seqNext) < 0) return;
    const Tone& t = g_seq[g_seqIndex++];
    if (t.freqHz) M5.Speaker.tone(t.freqHz, t.ms);   // tone() returns immediately
    g_seqNext = now + t.ms;
}

const uint32_t FRAME_MS = 33;   // ~30 fps
}  // namespace

void platformPlaySound(const Tone* tones, uint8_t count) {
    if (!isSoundEnabled()) return;
    if (count > MAX_TONES) count = MAX_TONES;
    for (int i = 0; i < count; ++i) g_seq[i] = tones[i];
    g_seqCount = count;
    g_seqIndex = 0;
    g_seqNext = millis();
    soundTick();
}

uint32_t platformLoadBest() { return prefs.getUInt("towerBest", 0); }
void     platformSaveBest(uint32_t best) { prefs.putUInt("towerBest", best); }

// ---------------------------------------------------------------------------
// Diagnostic: plays by itself (a drop every 0.7 s) for 25 s and prints how
// long update / render / push take each second. Started by sending 'b' over
// the serial port while the selector is showing.
void runTowerBenchmark() {
    M5.Display.setRotation(0);
    TowerApp* app = new TowerApp(esp_random(), "FRONT", "SIDE", true);
    uint32_t start = millis(), last = start, nextPress = start + 500, secStart = start;
    uint32_t frames = 0, tU = 0, tR = 0, tP = 0, tM = 0, worstM = 0, worst = 0;
    IdleTimer idle;
    idleStart(idle);
    Serial.println("BENCH start");
    while (millis() - start < 60000) {
        uint32_t f0 = millis();
        uint32_t m0 = micros();
        M5.update();
        bool ba = M5.BtnA.wasPressed(), bb = M5.BtnB.wasPressed();
        idleUpdate(idle, ba, bb);
        uint32_t m1 = micros();
        tM += m1 - m0;
        if (m1 - m0 > worstM) worstM = m1 - m0;
        // Autopilot: start/restart on a timer; while playing, drop when lined up.
        const TowerGame& g = app->game();
        bool press = false;
        if ((int32_t)(f0 - nextPress) >= 0) { press = true; nextPress = f0 + 1500; }
        if (g.layerCount() > 1 || press) {
            const Slab& top = g.layer(g.layerCount() - 1);
            float off = g.slider().x - top.x;
            if (off > -3.0f && off < 3.0f && !g.isOver()) { press = true; nextPress = f0 + 1500; }
        }
        float dt = (f0 - last) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last = f0;
        uint32_t a = micros();
        app->update(dt, press, false);
        uint32_t b = micros();
        app->render(g_fb);
        uint32_t c = micros();
        pushFramebuffer();
        uint32_t d = micros();
        tU += b - a; tR += c - b; tP += d - c;
        if (d - a > worst) worst = d - a;
        ++frames;
        soundTick();
        if (millis() - secStart >= 1000) {
            Serial.printf("BENCH t=%2us height=%d%s frames=%u m5update=%.2fms(worst %.1f) update=%.2fms render=%.2fms push=%.2fms worst=%.1fms\n",
                          (unsigned)((millis() - start) / 1000), g.score(), g.isOver() ? " OVER" : "",
                          (unsigned)frames, tM / 1000.0f / frames, worstM / 1000.0f,
                          tU / 1000.0f / frames, tR / 1000.0f / frames, tP / 1000.0f / frames,
                          worst / 1000.0f);
            secStart = millis();
            frames = tU = tR = tP = tM = worstM = worst = 0;
        }
        while (millis() - f0 < FRAME_MS) { soundTick(); delay(1); }
    }
    delete app;
    Serial.println("BENCH done");
}

void runTower() {
    M5.Display.setRotation(0);   // portrait
    static TowerApp app(esp_random(), "FRONT", "SIDE", true);
    app.enterTitle();

    IdleTimer idle;
    idleStart(idle);
    uint32_t last = millis();
    uint32_t statStart = last, statFrames = 0, statWork = 0, statWorst = 0;
    bool wasOver = false;

    for (;;) {
        uint32_t frameStart = millis();
        M5.update();
        bool a = M5.BtnA.wasPressed();
        bool b = M5.BtnB.wasPressed();
        bool wasDimmed = idle.dimmed;
        idleUpdate(idle, a, b);
        if (!wasDimmed && idle.dimmed) app.pause();   // don't play on in the dark
        if (M5.Display.getRotation() != 0) M5.Display.setRotation(0);

        float dt = (frameStart - last) / 1000.0f;
        if (dt > 0.1f) dt = 0.1f;
        last = frameStart;

        app.update(dt, a, b);
        if (app.exitRequested()) {
            M5.Speaker.stop();
            return;
        }
        app.render(g_fb);
        pushFramebuffer();

        // Short status on the serial monitor: game over events, and a
        // frame-rate summary every 30 s.
        uint32_t work = millis() - frameStart;
        statWork += work;
        if (work > statWorst) statWorst = work;
        const TowerGame& g = app.game();
        if (g.isOver() != wasOver) {
            wasOver = g.isOver();
            if (wasOver) Serial.printf("Tower: game over at height %d\n", g.score());
        }
        if (++statFrames, millis() - statStart >= 30000) {
            Serial.printf("Tower: %.1f fps, frame work %.1f ms avg / %u ms worst\n",
                          statFrames * 1000.0f / (millis() - statStart),
                          statWork / (float)statFrames, (unsigned)statWorst);
            statStart = millis();
            statFrames = statWork = statWorst = 0;
        }

        // Wait out the rest of the frame, keeping the sound sequencer running.
        while (millis() - frameStart < FRAME_MS) {
            soundTick();
            delay(1);
        }
    }
}
