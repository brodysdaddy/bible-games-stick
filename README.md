# Bible Games for M5StickS3

Two games on one tiny handheld: an **M5StickS3** (ESP32-S3, 135×240 color screen, two buttons, speaker, battery). It boots into a game selector where each game has its own look. Slide between the cards and pick one.

| Game | Orientation | What it is |
|---|---|---|
| **Bible Trivia** | Landscape | 5 random multiple-choice questions from a pool of 500 (250 Old Testament, 250 New Testament). Results are revealed one by one with sounds, then a score fanfare. |
| **Tower of Babel** | Portrait | Drop sliding brick layers to build the tower. Overhangs get sliced off; perfect drops build combos that widen it again. The sky turns from sunset to starry night as you climb. |

Both games were first built and tested as Windows programs:
- [bible-trivia-stick](https://github.com/brodysdaddy/bible-trivia-stick): the trivia game, with a Windows console version.
- [tower-of-babel-windows](https://github.com/brodysdaddy/tower-of-babel-windows): the tower game, with a Windows prototype.

This repo combines them into one firmware for the stick.

## Controls

The stick has a **front** button (below the screen) and a **side** button.

| Screen | Front button | Side button |
|---|---|---|
| Game selector | Play / toggle sound / power off | Next card |
| Bible Trivia | Select answer or menu item | Next choice |
| Tower of Babel | Start, drop a layer, play again | Pause/resume; on the title screen, back to the selector |

When the stick is off, a short press of the side button turns it on.

## Features

- **Game selector** with four themed full-screen cards and a sliding transition: Bible Trivia (navy and gold), Tower of Babel (sunset), Sound on/off, and Power off.
- **Bible Trivia:**
  - Answer choices are shuffled every time.
  - Your score stays hidden until the end.
  - Then each question is revealed with a correct or wrong sound, followed by a fanfare that matches your score.
  - Afterwards: retake the same test (with new choice order), take a new test, or go back to the menu.
- **Tower of Babel:**
  - A sky that changes with height, with parallax clouds and ziggurats.
  - Brick materials that change every 10 layers: mud, terracotta, glazed blue, turquoise, gold.
  - Tumbling cut-off pieces, dust and sparkle particles, and screen shake.
  - A chime that rises in pitch with each perfect in a row.
  - High score saved to flash.
- **Shared settings:**
  - One sound setting for both games, saved to flash.
  - Battery gauge on the selector.
  - The screen dims after 30 seconds idle.
  - On battery, it powers off after 3 minutes idle. Plugged into USB, it never auto-powers off.

## Architecture

```
bible_games/              The Arduino sketch: everything specific to the stick
  bible_games.ino         Entry point
  selector.cpp            Game selector (portrait)
  stick_app.cpp           Setup, power/idle handling, trivia screens (landscape)
  tower_stick.cpp         Tower loop: buttons, 30 fps timing, display, speaker
  sound_m5.cpp            Trivia sound playback
  stick_common.h          Shared declarations
lib/
  TriviaCore/             Trivia rules, 500 questions, sounds as data
  TowerOfBabel/src/       Tower rules, 135x240 framebuffer, visuals, sounds
```

The games in `lib/` are **portable C++** with no platform code and no heap allocation. They're the same source as the Windows versions, which is why both games could be developed and tested on a PC first.

- **Bible Trivia** draws with M5GFX into an off-screen sprite.
- **Tower of Babel** draws everything into its own 135×240 RGB565 buffer, the stick's native screen format. The stick sends it to the display about 30 times a second. Each frame takes about 4 ms to draw and 13 ms to send, well within the 33 ms budget.
- **Sounds** are data (lists of frequency and duration). Trivia plays its short sounds directly. The tower game uses a small sequencer that runs between frames, so the action never stops for a sound.

**Memory:** the firmware is about 725 KB, around 21% of flash. Each trivia question costs about 94 bytes, so there is room for tens of thousands more.

## Building and flashing

Uses [arduino-cli](https://arduino.github.io/arduino-cli/).

**One-time setup:**

```bat
arduino-cli config add board_manager.additional_urls https://static-cdn.m5stack.com/resource/arduino/package_m5stack_index.json
arduino-cli core update-index
arduino-cli core install m5stack:esp32
arduino-cli lib install M5Unified
```

**Compile and upload.** Check Device Manager for the stick's COM port.

```bat
upload.bat          :: COM4
upload.bat COM5
```

If the upload can't connect, hold the side button about 2 seconds until the green LED blinks (download mode) and run it again.

**Using the Arduino IDE instead:** copy `lib/TriviaCore` and `lib/TowerOfBabel` into your `Documents/Arduino/libraries` folder, open `bible_games/bible_games.ino`, and select the board **M5StickS3**.

## Diagnostics

Open a serial monitor at 115200 baud to see game-over events and a frame-rate summary every 30 seconds. While the game selector is showing, send `b` to run a 60-second benchmark. The stick plays the tower game by itself and reports how long each part of a frame takes.

## Future ideas

- **Voice answers** for trivia, using the stick's microphone.
- **More questions**, generated from a spreadsheet.
- **Old/New Testament mix:** choose OT only, NT only, or mixed.
- **More games** in the selector: the card system makes adding another one simple.
- **Tilt controls** using the stick's motion sensor.
