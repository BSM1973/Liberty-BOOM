#pragma once
#include <JuceHeader.h>

// Shared native typography and colour palette for BOOM. One font family is
// used for the header, both toolbar groups, the factory browser, the user
// browser, mastering-chain tabs and the auxiliary measurements/settings pages.
// French text must be written using C++ Unicode escape sequences in sources,
// so no runtime code-page conversion or system-specific font is necessary.
namespace boom::ui::theme {
inline juce::Font font(float size,bool bold=false) {
 return juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain));
}
inline juce::Colour background() { return juce::Colour::fromRGB(23,10,13); }
inline juce::Colour surface()    { return juce::Colour::fromRGB(35,17,21); }
inline juce::Colour active()     { return juce::Colour::fromRGB(108,26,38); }
inline juce::Colour copper()     { return juce::Colour::fromRGB(174,89,70); }
inline juce::Colour gold()       { return juce::Colour::fromRGB(224,188,125); }
inline juce::Colour ivory()      { return juce::Colour::fromRGB(246,228,195); }
inline juce::Colour muted()      { return juce::Colour::fromRGB(190,164,134); }

inline constexpr float caption=10.5f;
inline constexpr float label=12.f;
inline constexpr float body=13.5f;
inline constexpr float heading=19.f;
}
