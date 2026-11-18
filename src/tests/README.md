# XTEngine Unit Tests

Standalone Qt Test suite for the C++ library. Test sources are compiled
directly into the test app (no full library build needed), so the suite only
requires Qt (core, gui, testlib, multimedia) — no ffmpeg usage, no other
system dependencies.

## Run

Linux/macOS:

    src/tests/run_tests.sh

or manually:

    cd src/tests/build
    qmake ../XTEngineTests.pro
    make
    ./XTEngineTests

Windows / Qt Creator: open `src/tests/XTEngineTests.pro` as a project and run.

Note: on minimal systems the test binary may fail to start with
`libgssapi_krb5.so.2: cannot open shared object file` (a runtime dependency
of Qt6Network). Point `LD_LIBRARY_PATH` at a directory containing the
Kerberos libs (libgssapi_krb5, libkrb5, libk5crypto, libkrb5support,
libkeyutils) to run the suite.

GitHub Actions: install Qt (e.g. `qt6` via the `qt/qtdownload` setup or a
prebuilt kit), then run the same qmake/make/execute steps.

## Adding tests

- New test class: add `t_<name>.h/.cpp` and register the source in
  `XTEngineTests.pro` (SOURCES/HEADERS).
- New class under test: add its `.cpp` to SOURCES in `XTEngineTests.pro`.
- Test fixtures: commit binary files under `data/` (tests must not generate
  them at runtime).

## Current coverage

`t_imagefactory.cpp` — `ImageFactory::isAnimatedImageFile()` (animated/static
GIF, WebP, APNG/PNG, AVIF + missing/resource/empty path edge cases) and
`ImageFactory::staticFirstFrameJpeg()` (valid JPEG output, graceful empty
fallbacks, cache hit, cache invalidation on file change).

`t_funscripthandler.cpp` — the amplitude modifier (local-center scaling,
clamping, roaming bands, flat runs, single action, final point) and the
`getPosition()` action stream, plus the script metadata parsing:
`parseScriptInfo()` (full metadata, absent properties keep defaults,
explicit `inverted`/`paid` false vs absent, wrong JSON types ignored,
duration from number/string, regular files, SFMA `channels[track]`
targeting, missing-track fallback, missing/invalid files) and
`readScriptJson()` (valid/missing/invalid file).

`t_scriptinfo.cpp` — `ScriptInfo` `==`/`!=` (identical, and a data-driven
test flipping each metadata field one at a time) and serialization
(`toJson`/`fromJson` round-trip, old saved metadata without the new keys
loads with defaults, QDataStream round-trip, `FunscriptMetadata`
`toJson`/`fromJson` round-trip).
