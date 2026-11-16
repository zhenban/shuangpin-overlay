# Contributing

Bug reports and pull requests are welcome.

## Building and testing

```sh
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Layout

| Path | What is there |
| --- | --- |
| `src/pinyin.*` | Which syllables exist |
| `src/scheme.*`, `src/schemelibrary.*` | How a scheme maps syllables to keys, and the set of schemes available |
| `src/composition.*` | The composition in progress; finds slips and times finals |
| `src/typingtracker.*` | Turns key presses into edits of the composition |
| `src/visibilitypolicy.*` | The rules for when the chart is on screen |
| `src/practicestats.*` | The practice figures and their file |
| `src/keyboardmonitor.*` | Reads key presses from `/dev/input` |
| `src/inputmethodwatcher.*` | Asks Fcitx5 for the active input method and scheme |
| `src/chartpainter.*`, `src/keyhighlights.*` | Draws the chart; which keys are lit |
| `src/windowplacement.*` | Puts the window on screen: layer-shell overlay or ordinary window |
| `src/overlaymenu.*` | The context menu |
| `src/overlaywindow.*` | Owns the parts above and connects them; fading and mouse handling |
| `data/schemes/` | Built-in schemes |
| `translations/` | Interface translations |
| `tests/` | Unit tests |
| `packaging/` | The RPM spec, and `build-rpm.sh` to build it from the committed tree |
| `docs/` | Pictures for the README; `docs/generate.sh` makes them from the program |

Everything down to `practicestats` is in the `shuangpin-core` library. It needs no display, and it is what the
tests link against. New logic belongs there when it can: decide in the core, and leave the window code to
apply the decision.

## Conventions

- C++20 and Qt 6. Format with the `.clang-format` in the repository.
- User-visible strings go through `tr()` and are written in English. After changing them, run
  `cmake --build build --target update_translations` and fill in `translations/*.ts`.
- A new scheme is a JSON file in `data/schemes/` plus a line in the completeness test in
  `tests/test_scheme.cpp`, and a section in `docs/schemes.md`.
- After a change to how the chart looks, run `docs/generate.sh` and commit the pictures.
- Nothing that was typed may be written to disk or leave the process. Statistics are aggregates only.

## Releasing

1. Set the version in `CMakeLists.txt` and `packaging/shuangpin-overlay.spec`, and add a `<release>` to
   `data/app.metainfo.xml.in` and a `%changelog` entry to the spec.
2. Give the version a section in `CHANGELOG.md`; it becomes the text of the release.
3. Commit, then tag and push: `git tag v0.2.0 && git push origin main v0.2.0`.

The workflow builds the RPMs for the Fedora versions listed in it and publishes the release. It refuses a tag that does not match the version.
