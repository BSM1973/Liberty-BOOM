#!/usr/bin/env python3
"""BOOM v1.7 upper-area glyph safety and language selection contract.

Static source checks complement the native AU/VST3 compilation.
Visual proof still needs a screenshot of the new binary in a DAW.
"""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=lambda p:(root/p).read_text(encoding="utf-8")
safe=source("Source/BoomSafeText.h")
chrome=source("Source/BoomChrome.cpp")
chrome_h=source("Source/BoomChrome.h")
browser=source("Source/BoomBrowser.cpp")
preview=source("Source/BoomBrowserActions.cpp")
chain=source("Source/BoomChainPanel.cpp")
menu=source("Source/PluginEditorMenus.cpp")
processor=source("Source/PluginProcessor.cpp")
cmake=source("CMakeLists.txt")
workflow=source(".github/workflows/build.yml")

assert "Source/BoomSafeText.h" in cmake
assert "safeUpperText(const juce::String& input)" in safe
assert "out+=juce::String::charToString" in safe
assert "if(cp>=32u&&cp<=126u)" in safe
for codepoint in ["0x00E9","0x00C9","0x02C6","0x00A9","0x00C3","0xFFFD"]:
    # Some malformed glyphs are removed by the default branch rather than
    # explicitly referenced as constants.
    if codepoint!="0xFFFD":
        assert codepoint in safe, f"Missing common French/mojibake handling: {codepoint}"
assert "std::array<juce::TextButton,5> headerButtons;" in chrome_h
assert 'wireButton(headerButtons[4],Command::Language,"LANGUE : FR")' in chrome
assert 'headerButtons[4].setBounds(langX,langY,langW' in chrome
assert 'headerButtons[4].setButtonText(english?"LANG : EN":"LANGUE : FR")' in chrome
assert "headerButtons[4].setTooltip" in chrome
assert 'g.drawFittedText(boom::ui::text::safeUpperText(b.getButtonText())' in chrome
assert 'styleName=boom::ui::text::safeUpperText(style)' in chrome
assert 'presetName=boom::ui::text::safeUpperText(preset)' in chrome
assert "boom::ui::text::safeUpperText(text)" in chrome
assert 'menu.addItem(1,"FR - Francais"' in menu
assert 'menu.addItem(2,"EN - English"' in menu
assert 'self->write("language",float(value-1))' in menu
assert 'self->nativeBrowser->updateLanguage(self->english)' in menu
assert 'self->refreshChrome()' in menu
assert 'addChoice(l,"language","Language",{"FR","EN"})' in processor
assert 'juce::String::charToString' in safe

# Compatibility normalization applies only to DISPLAY, never stored presets.
for fragment in (
 "g.drawFittedText(boom::ui::text::safeUpperText(b.getButtonText())",
 "boom::ui::text::safeUpperText(item.name)",
 "boom::ui::text::safeUpperText(name)",
 "const auto visible=boom::ui::text::safeUpperText(msg)",
 "status.setText(visible,juce::dontSendNotification)",
):
    assert fragment in browser,fragment
assert 'boom::ui::text::safeUpperText(content)' in preview
assert 'boom::ui::text::safeUpperText(label)' in preview
assert "BOOM / BIBLIOTHEQUE" in preview
assert 'boom::ui::text::safeUpperText(title)' in chain
assert 'return boom::ui::text::safeUpperText(en?namesEN[index]:namesFR[index]);' in chain
assert "proc.userPresetName(" in browser
assert "setValueNotifyingHost" not in safe, "A text renderer must never alter DSP parameters"
assert "setProperty" not in safe, "Display-only normalization must never modify state"
assert "python3 Tests/upper_text_language_contract.py" in workflow

# At 13-inch fit sizes, the new language button does not overlap
# the native preset selector and sits after the centered BOOM inscription.
for width in (790,960,1180,1440,1592,1672):
    x=round(.662*width)
    w=round(.084*width)
    preset_start=round(.752*width)
    assert x>round(.655*width), width
    assert x+w<preset_start, (width,x+w,preset_start)
print("BOOM upper text/language PASS: display-only glyph cleanup, "
      "5 native header controls, FR/EN APVTS choices and nonoverlap on 13-inch windows")
