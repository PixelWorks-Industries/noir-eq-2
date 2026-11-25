# NOIR EQ visual assets

All assets in this folder are original project-authored SVGs. They are not scraped stock images and require no external runtime downloads.

- `noir_plate.svg`: subtle machined plate / hairline / micro-grain texture, embedded as the editor background.
- `noir_wordmark.svg`: custom NOIR / EQ 01 lockup, embedded in the header.
- `noir_mark.svg`: compact monogram source mark.
- `noir_meter_ticks.svg`: vector meter scale artwork for future meter refinements.
- `noir_surface.svg`: alternate fine-grain surface texture.

CMake compiles the SVG files into JUCE BinaryData, so they ship inside the plugin binary.
