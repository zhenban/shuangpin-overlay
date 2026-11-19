# Changelog

## 0.1.3 (2026-11-20)

- Overlay mode: dragging is smooth, at the display's frame rate
- The chart no longer hides while it is handled, or the moment it is let go; catching it as it fades out
  brings it back
- README: keeping the chart on every virtual desktop in KDE Plasma

## 0.1.2 (2026-11-16)

- The chart no longer hides while it is being dragged, or when a click on it takes the keyboard focus
- The window carries its own icon, and its title is the application's name

## 0.1.1 (2026-11-16)

No change to the program.

- Packages for Fedora 44 and 45. The 0.1.0 release only had one for Fedora 45.
- Fixed the build without LayerShellQt.

## 0.1.0 (2026-11-16)

First version.

- Floating key chart for eight Shuangpin schemes, following Fcitx5's setting
- Key highlighting, readout of the typed syllables, and hints for the finals that can follow an initial
- Shows only while a Shuangpin input method is active; hides when the keyboard is idle
- "Only appear when I get stuck" mode
- Practice statistics: slowest finals and frequently mixed-up keys
- Optional overlay mode through layer-shell where the compositor supports it
- `--export` saves the chart as a picture, optionally after a given sequence of keys
- English and Simplified Chinese interface
