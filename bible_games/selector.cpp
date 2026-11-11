// selector.cpp - the game selector shown at startup (portrait, 135x240).
//
// One full-screen card at a time, each with its own look:
//   Bible Trivia (navy + yellow), Tower of Babel (sunset), Sound, Power off.
// Side button slides to the next card; front button picks it.
#include "stick_common.h"
#include "sound_player.h"
#include <math.h>
#include <stdio.h>

namespace {

enum Card { CARD_TRIVIA, CARD_TOWER, CARD_SOUND, CARD_POWER, NUM_CARDS };

const int   SW = Framebuffer::W;
const int   SH = Framebuffer::H;
const int   SLIDE_MS = 260;           // card transition length
const Rgb   WHITE = {255, 255, 255};
const Rgb   GREY  = {170, 170, 185};
const Rgb   YELLOW = {255, 230, 0};
const Rgb   GOLD  = {255, 222, 110};

int g_card = CARD_TRIVIA;              // remembered between visits

// Vertical gradient filling one card (cards slide, so everything takes xo).
void gradient(int xo, Rgb top, Rgb bot) {
    for (int y = 0; y < SH; ++y) g_fb.hline(xo, y, SW, to565(lerpRgb(top, bot, y / (float)(SH - 1))));
}
Rgb gradientAt(Rgb top, Rgb bot, int y) { return lerpRgb(top, bot, y / (float)(SH - 1)); }

void textC(int xo, int y, const char* s, Rgb c, int scale) {
    int x = xo + (SW - Framebuffer::textWidth(s, scale)) / 2;
    g_fb.text(x + 1, y + 1, s, rgb565(0, 0, 0), scale, 140);   // soft shadow
    g_fb.text(x, y, s, to565(c), scale);
}

// ---- Cards ----------------------------------------------------------------

void drawTriviaCard(int xo) {
    gradient(xo, {8, 8, 60}, {0, 0, 140});
    // Open book.
    const int cx = xo + SW / 2, top = 46;
    const uint16_t page = rgb565(245, 236, 210), line = rgb565(170, 160, 140);
    const uint16_t cover = rgb565(120, 60, 30);
    g_fb.fillRect(cx - 44, top + 44, 88, 5, cover);
    g_fb.fillRect(cx - 40, top, 38, 46, page);
    g_fb.fillRect(cx + 2, top, 38, 46, page);
    g_fb.fillRect(cx - 2, top - 2, 4, 51, cover);
    for (int k = 0; k < 6; ++k) {
        g_fb.hline(cx - 36, top + 7 + k * 6, 30, line);
        g_fb.hline(cx + 6, top + 7 + k * 6, 30, line);
    }
    g_fb.fillRect(cx + 22, top + 46, 3, 12, rgb565(200, 30, 40));   // ribbon
    textC(xo, 122, "BIBLE", YELLOW, 3);
    textC(xo, 148, "TRIVIA", YELLOW, 3);
    textC(xo, 178, "500 QUESTIONS", GREY, 1);
}

void drawTowerCard(int xo) {
    gradient(xo, {88, 58, 140}, {255, 158, 92});
    g_fb.blendCircle(xo + 38, 160, 26, rgb565(255, 200, 120), 50);
    g_fb.fillCircle(xo + 38, 160, 17, rgb565(255, 224, 150));
    // Distant land with a ziggurat, then sand.
    const uint16_t land = rgb565(120, 62, 78);
    g_fb.fillRect(xo, 166, SW, 10, land);
    for (int s = 0; s < 3; ++s) g_fb.fillRect(xo + 92 + s * 5, 166 - (s + 1) * 5, 34 - s * 10, 5, land);
    g_fb.fillRect(xo, 174, SW, SH - 174, rgb565(206, 164, 104));
    g_fb.hline(xo, 174, SW, rgb565(232, 196, 136));
    // A little brick tower.
    const int widths[8] = { 72, 68, 66, 60, 58, 52, 50, 46 };
    const int shifts[8] = { 0, 2, -1, 3, 1, -2, 2, 0 };
    for (int i = 0; i < 8; ++i) {
        Rgb c = i < 4 ? Rgb{198, 146, 90} : Rgb{186, 98, 62};
        int w = widths[i], x = xo + 68 - w / 2 + shifts[i], y = 164 - i * 10;
        uint16_t mortar = to565(shade(c, -45));
        g_fb.fillRect(x, y, w, 10, to565(c));
        g_fb.hline(x, y, w, to565(shade(c, 35)));
        g_fb.hline(x, y + 5, w, mortar);
        for (int jx = x + 6 + (i % 2) * 6; jx < x + w - 1; jx += 12) g_fb.vline(jx, y + 1, 4, mortar);
        for (int jx = x + 12 - (i % 2) * 6; jx < x + w - 1; jx += 12) g_fb.vline(jx, y + 6, 4, mortar);
        g_fb.vline(x + w - 1, y, 10, to565(shade(c, -35)));
    }
    textC(xo, 18, "TOWER", GOLD, 3);
    textC(xo, 46, "OF BABEL", GOLD, 2);
    char buf[20];
    snprintf(buf, sizeof buf, "BEST %u", (unsigned)prefs.getUInt("towerBest", 0));
    textC(xo, 186, buf, WHITE, 1);
}

void drawSoundCard(int xo) {
    const Rgb top = {10, 40, 50}, bot = {0, 95, 105};
    gradient(xo, top, bot);
    const int cx = xo + SW / 2 - 10, cy = 92;
    const uint16_t white = to565(WHITE);
    g_fb.fillRect(cx - 24, cy - 12, 14, 24, white);                  // speaker body
    const float xs[4] = { (float)(cx - 11), (float)(cx + 8), (float)(cx + 8), (float)(cx - 11) };
    const float ys[4] = { (float)(cy - 12), (float)(cy - 30), (float)(cy + 30), (float)(cy + 12) };
    g_fb.fillQuad(xs, ys, white);                                    // cone
    if (isSoundEnabled()) {
        for (int r = 16; r <= 32; r += 8)                            // sound waves
            for (int deg = -40; deg <= 40; deg += 2) {
                float a = deg * 3.14159f / 180.0f;
                int x = cx + 10 + (int)(r * cosf(a)), y = cy + (int)(r * sinf(a));
                g_fb.fillRect(x, y, 3, 3, white);
            }
    } else {
        const uint16_t red = rgb565(255, 80, 80);
        for (int k = -10; k <= 10; ++k) {                            // an X
            g_fb.fillRect(cx + 26 + k, cy + k, 4, 4, red);
            g_fb.fillRect(cx + 26 + k, cy - k, 4, 4, red);
        }
    }
    textC(xo, 140, "SOUND", WHITE, 3);
    if (isSoundEnabled()) textC(xo, 170, "ON", {120, 255, 140}, 3);
    else                  textC(xo, 170, "OFF", {255, 110, 110}, 3);
}

void drawPowerCard(int xo) {
    const Rgb top = {40, 6, 10}, bot = {110, 20, 24};
    gradient(xo, top, bot);
    const int cx = xo + SW / 2, cy = 92;
    const uint16_t white = to565(WHITE);
    g_fb.fillCircle(cx, cy, 27, white);                              // power symbol
    g_fb.fillCircle(cx, cy, 21, to565(gradientAt(top, bot, cy)));
    g_fb.fillRect(cx - 7, cy - 30, 14, 22, to565(gradientAt(top, bot, cy - 25)));
    g_fb.fillRect(cx - 3, cy - 32, 6, 30, white);
    textC(xo, 140, "POWER", WHITE, 3);
    textC(xo, 170, "OFF", WHITE, 3);
}

void drawCard(int card, int xo) {
    switch (card) {
    case CARD_TRIVIA: drawTriviaCard(xo); break;
    case CARD_TOWER:  drawTowerCard(xo);  break;
    case CARD_SOUND:  drawSoundCard(xo);  break;
    default:          drawPowerCard(xo);  break;
    }
}

// Fixed on top of the sliding cards: battery, page dots, button hints.
void drawChrome(int card) {
    int level = M5.Power.getBatteryLevel();
    if (level >= 0 && level <= 100) {
        char buf[8];
        snprintf(buf, sizeof buf, "%s%d%%", M5.Power.isCharging() ? "+" : "", level);
        int bx = SW - 18, by = 3;
        uint16_t col = level > 50 ? rgb565(80, 220, 100) : level > 20 ? rgb565(240, 210, 60)
                                                                      : rgb565(240, 70, 70);
        uint16_t frame = rgb565(210, 210, 210);
        g_fb.hline(bx, by, 14, frame); g_fb.hline(bx, by + 7, 14, frame);
        g_fb.vline(bx, by, 8, frame);  g_fb.vline(bx + 13, by, 8, frame);
        g_fb.fillRect(bx + 14, by + 2, 2, 4, frame);
        g_fb.fillRect(bx + 2, by + 2, 10 * level / 100, 4, col);
        g_fb.text(bx - 3 - Framebuffer::textWidth(buf), by, buf, frame);
    }
    const int dotY = 208, spacing = 12, x0 = SW / 2 - (NUM_CARDS - 1) * spacing / 2;
    for (int i = 0; i < NUM_CARDS; ++i)
        g_fb.fillCircle(x0 + i * spacing, dotY, i == card ? 3 : 2,
                        i == card ? to565(WHITE) : rgb565(150, 150, 160));

    static const char* const ACTION[NUM_CARDS] = {
        "FRONT: PLAY", "FRONT: PLAY", "FRONT: ON/OFF", "FRONT: POWER OFF" };
    textC(0, 218, ACTION[card], WHITE, 1);
    textC(0, 229, "SIDE: NEXT", GREY, 1);
}

void showCard(int card) {
    drawCard(card, 0);
    drawChrome(card);
    pushFramebuffer();
}

// Slide the current card out to the left and the next one in.
void slideTo(int from, int to) {
    uint32_t start = millis();
    for (;;) {
        float t = (millis() - start) / (float)SLIDE_MS;
        if (t >= 1) break;
        float e = 1 - (1 - t) * (1 - t) * (1 - t);   // ease out
        int off = (int)(e * SW);
        drawCard(from, -off);
        drawCard(to, SW - off);
        drawChrome(to);
        pushFramebuffer();
    }
}

}  // namespace

GameChoice runSelector() {
    M5.Display.setRotation(0);   // portrait
    IdleTimer idle;
    idleStart(idle);
    showCard(g_card);
    uint32_t lastRefresh = millis();

    for (;;) {
        if (Serial.available() && Serial.read() == 'b') {   // diagnostic benchmark
            runTowerBenchmark();
            M5.Display.setRotation(0);
            showCard(g_card);
            idleStart(idle);
        }
        M5.update();
        bool a = M5.BtnA.wasPressed();
        bool b = M5.BtnB.wasPressed();
        idleUpdate(idle, a, b);
        if (M5.Display.getRotation() != 0) {    // a failed power-off changes it
            M5.Display.setRotation(0);
            showCard(g_card);
        }

        if (b) {
            playSound(SoundId::Click);
            int next = (g_card + 1) % NUM_CARDS;
            slideTo(g_card, next);
            g_card = next;
            showCard(g_card);
        } else if (a) {
            playSound(SoundId::Click);
            switch (g_card) {
            case CARD_TRIVIA: return GameChoice::Trivia;
            case CARD_TOWER:  return GameChoice::Tower;
            case CARD_SOUND:
                setSoundEnabled(!isSoundEnabled());
                prefs.putBool("sound", isSoundEnabled());
                playSound(SoundId::Click);
                showCard(g_card);
                break;
            default:
                powerOffNow();
                M5.Display.setRotation(0);
                showCard(g_card);
                break;
            }
        }
        if (millis() - lastRefresh > 5000) {    // keep the battery reading fresh
            showCard(g_card);
            lastRefresh = millis();
        }
        delay(10);
    }
}
