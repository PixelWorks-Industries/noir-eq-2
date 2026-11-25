# NOIR EQ

A dark, hardware-influenced three-band EQ and compressor VST3 built with JUCE 8.

## Features

- Three-band EQ: low shelf, parametric mid band, and high shelf.
- Compressor with threshold, ratio, attack, release, and makeup gain.
- Draggable EQ response nodes.
- 64-band FFT spectrum display.
- Input/output peak meters.
- Compressor gain-reduction estimate display (`GR EST.`). This is an estimate based on the input block peak and static ratio curve, not the compressor's internal detector telemetry.
- Original SVG artwork embedded into the plugin with JUCE BinaryData.
- APVTS parameter state save/restore and host automation.

## Build on GitHub Actions

1. Push the complete repository to GitHub, including `.github/workflows/build-vst3.yml`.
2. Open **Actions** and run **Build NOIR EQ (Windows VST3)**, or push a commit to `main`.
3. Download the `NOIR-EQ-Windows-x64-VST3` artifact from the completed workflow run.
4. Extract `NOIR EQ.vst3` into `C:\Program Files\Common Files\VST3\`.
5. Rescan plugins in FL Studio.

The workflow builds a Windows x64 Release VST3 using a GitHub-hosted Windows runner.

## Local Windows build

Requirements: Visual Studio 2022 with the C++ desktop workload, CMake 3.22+, and an internet connection for JUCE FetchContent.

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target NOIR_EQ_VST3 --parallel
```

The VST3 bundle is generated under `build\NOIR_EQ_artefacts\Release\VST3\`.

## Notes

- The project fetches JUCE 8.0.10 at configure time.
- The SVG files in `Assets/` are original project-authored vectors; no stock-image downloads or external runtime asset paths are used.
- The build has been source-checked against the compiler errors in the supplied GitHub Actions log. A fresh full compile still needs to be confirmed by GitHub Actions because this environment cannot reach GitHub to fetch JUCE.

## License

Review JUCE's current licensing terms before distributing binaries or using the project commercially.

## 0.2.1 build compatibility

The editor uses JUCE 8 `FontOptions` constructors and valid `Graphics::drawText` overloads. The unsupported `TextButton::outlineColourId` was removed. See `BUILD_FIXES.md` for the fixes mapped to the supplied MSVC errors.
