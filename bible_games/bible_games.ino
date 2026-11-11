// bible_games.ino - Bible Trivia + Tower of Babel for the M5StickS3.
//
// Build with upload.bat in the repo root, which passes the two game
// libraries in lib/ to arduino-cli.
//
//   selector.cpp     game selector shown at startup (portrait)
//   stick_app.cpp    setup, power/idle handling, Bible Trivia screens (landscape)
//   tower_stick.cpp  Tower of Babel loop, display, sound, high score (portrait)
//   sound_m5.cpp     trivia sound playback

void appSetup();
void appLoop();

void setup() { appSetup(); }
void loop()  { appLoop(); }
