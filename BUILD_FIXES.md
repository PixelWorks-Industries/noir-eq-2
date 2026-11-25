# Build fixes in 0.2.1

This revision addresses the JUCE 8 / MSVC compiler errors reported by GitHub Actions:

- Removed the unsupported `juce::TextButton::outlineColourId` reference. JUCE TextButton exposes `buttonColourId`, `buttonOnColourId`, `textColourOffId`, and `textColourOnId`.
- Converted every editor font construction to `juce::Font(juce::FontOptions(typeface, height, styleFlags))`.
- Added an explicit `juce::Justification` to every `Graphics::drawText` call using integer coordinates.
- Made AudioParameterFloat default values explicit `float` literals to avoid narrowing warnings.

The source was checked for remaining instances of the reported invalid API patterns. A successful Windows VST3 build must still be confirmed by the repository's GitHub Actions runner.

# Build fixes in 0.2.2

- Replaced every `juce::Font(juce::FontOptions("Arial", size, style))` with a `noirFont(size, bold)` helper that builds the font via `FontOptions().withName().withHeight().withStyle()`. This fixes MSVC C2440 (cannot convert initializer list to `juce::FontOptions`) and the C4996 deprecated `juce::Font` warnings on JUCE 8.0.10.

# Build fixes in 0.2.3

- Fixed MSVC `C1189` in `juce_audio_plugin_client_VST3.cpp` ("conflict with parameter automation between VST2 and VST3"). NOIR EQ never shipped as a VST2, so `CMakeLists.txt` now defines `JUCE_VST3_CAN_REPLACE_VST2=0` and `JUCE_IGNORE_VST3_MISMATCHED_PARAMETER_ID_WARNING=1`.
- Renamed the editor's file-scope `line` colour to `lineCol` to stop the C4459 "hides global declaration" warnings from JUCE headers.
