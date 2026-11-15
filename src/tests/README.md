# XTEngine Unit Tests

Standalone Qt Test suite for the C++ library. Test sources are compiled
directly into the test app (no full library build needed), so the suite only
requires Qt (core, gui, testlib) — no ffmpeg, no multimedia, no other
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
