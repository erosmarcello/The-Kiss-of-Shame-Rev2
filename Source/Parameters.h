#pragma once

// Stable parameter IDs for the AudioProcessorValueTreeState.
// Rev 2 preserves the Rev 1 parameter set exactly — the controls that always
// existed on the faceplate, now actually functional and automatable.
// Deliberately nothing new: Shame Extreme and the UI era are state
// *properties*, not parameters (see PluginProcessor).
namespace ParamIDs
{
    inline constexpr auto input        = "input";        // input drive (saturation)
    inline constexpr auto shame        = "shame";        // wow/flutter/drift macro
    inline constexpr auto hiss         = "hiss";         // tape noise level
    inline constexpr auto age          = "age";          // degradation intensity
    inline constexpr auto blend        = "blend";        // dry/wet
    inline constexpr auto output       = "output";       // output level
    inline constexpr auto flange       = "flange";       // reel-touch flange depth
    inline constexpr auto bypass       = "bypass";
    inline constexpr auto tapeType     = "tapeType";     // S-111 / A-456
    inline constexpr auto printThrough = "printThrough";
    inline constexpr auto environment  = "environment";  // storage environment
}

namespace StateIDs
{
    inline constexpr auto shameExtreme = "shameExtreme"; // double-click the Shame knob
    inline constexpr auto uiEra        = "uiEra";        // "heritage" | "modern"
    inline constexpr auto showReels    = "showReels";
    inline constexpr auto uiScale      = "uiScale";      // editor zoom, 0.6 - 2.0
}
