# 🖥️ Productivity Dashboard++ — C++ & SFML

A feature-rich desktop productivity dashboard built in C++ using the SFML graphics library. The app runs as a single 800×600 window with a tabbed navigation bar giving access to six interactive widgets.

---

## ✨ Features

| # | Widget | Description |
|---|--------|-------------|
| 1 | 🕐 **Clock** | Live digital clock with 12/24-hour toggle |
| 2 | 🌤️ **Weather** | Mock weather display (temperature, humidity, condition) |
| 3 | 📝 **Notes** | Password-protected notes with add, edit, and delete — persisted to `notes_v2.txt` |
| 4 | 📅 **Calendar** | Monthly calendar view with custom event support |
| 5 | ⏰ **Alarm** | Set alarms with AM/PM selection; plays a sound and flashes on trigger |
| 6 | ⏱️ **Stopwatch** | Start/stop/reset stopwatch with lap tracking |

---

## 🗂️ Project Structure

```
SERIOUSPODOMORE/
├── main.cpp              # Full source code (single-file architecture)
├── dashboard_app.exe     # Pre-built Windows executable
├── background.png        # Background image asset
├── arial.ttf             # Font used throughout the UI
├── alarm.wav             # Alarm sound effect
├── notes_v2.txt          # Persistent notes storage
├── libgcc_s_seh-1.dll    # MinGW runtime
├── libstdc++-6.dll       # MinGW runtime
├── libwinpthread-1.dll   # MinGW runtime
├── sfml-graphics-2.dll   # SFML Graphics module
├── sfml-audio-2.dll      # SFML Audio module
├── sfml-window-2.dll     # SFML Window module
├── sfml-system-2.dll     # SFML System module
└── sfml-network-2.dll    # SFML Network module
```

---

## 🚀 Running the App (Windows)

A pre-built executable is included. Just make sure all `.dll` files are in the **same folder** as `dashboard_app.exe`, then double-click to run.

> ⚠️ Do not move the `.exe` out of its folder — it depends on the DLLs and asset files next to it.

---

## 🔨 Building from Source

### Prerequisites

- [SFML 2.x](https://www.sfml-dev.org/download.php) (Graphics, Audio, Window, System)
- A C++17-compatible compiler (e.g. MinGW-w64 / g++)

### Compile (MinGW example)

```bash
g++ main.cpp -o dashboard_app \
  -lsfml-graphics -lsfml-audio -lsfml-window -lsfml-system \
  -std=c++17
```

Make sure `arial.ttf`, `background.png`, `alarm.wav`, and `notes_v2.txt` are in the same directory as the compiled binary.

---

## ⌨️ Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `1` | Switch to Clock |
| `2` | Switch to Weather |
| `3` | Switch to Notes |
| `4` | Switch to Calendar |
| `5` | Switch to Alarm |
| `6` | Switch to Stopwatch |

---

## 🛠️ Tech Stack

- **Language:** C++17
- **Graphics & Audio:** [SFML 2](https://www.sfml-dev.org/)
- **Architecture:** Single-file, widget-based with an abstract `Widget` base class
- **File I/O:** Notes are saved/loaded from a plain text file (`notes_v2.txt`)

---

## 📌 Notes

- The weather widget uses **mock/randomized data** — no live API is connected.
- The Notes widget is **password-protected**; you'll be prompted on first access.
- The app window is fixed at **800 × 600**.
