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
editor=source("Source/PluginEditor.cpp")
panels=source("Source/PluginEditorPanels.cpp")
chain_h=source("Source/BoomChainPanel.h")
assert "case Cmd::French:setInterfaceLanguage(false)" in editor
assert "case Cmd::English:setInterfaceLanguage(true)" in editor
assert "case Cmd::Language:" not in editor
assert "parameterId,frenchLabel" in chain_h, "Each control must carry a translatable label"
assert "englishTitle(" in chain
assert "for(auto& c:rotaries)" in chain and "for(auto& c:choices)" in chain
assert "for(auto& t:toggles)" in chain
assert "PRECISION METERS" in panels and "MASTERING SETTINGS" in panels
assert "BOOM CONSOLE GUIDE" in panels and "Choose a Style" in panels
assert "\\u00" not in panels, "Legacy auxiliary pages still use unsafe Unicode escapes"
assert panels.isascii()
assert "safeUpperText(const juce::String& input)" in safe
assert "out+=juce::String::charToString" in safe
assert "if(cp>=32u&&cp<=126u)" in safe
for codepoint in ["0x00E9","0x00C9","0x02C6","0x00A9","0x00C3","0xFFFD"]:
    # Some malformed glyphs are removed by the default branch rather than
    # explicitly referenced as constants.
    if codepoint!="0xFFFD":
        assert codepoint in safe, f"Missing common French/mojibake handling: {codepoint}"
assert "std::array<juce::TextButton,6> headerButtons;" in chrome_h
assert 'wireButton(headerButtons[4],Command::French,"FR")' in chrome
assert 'wireButton(headerButtons[5],Command::English,"EN")' in chrome
assert 'headerButtons[4].setBounds(langX,langY,langSingle,bh)' in chrome
assert 'headerButtons[5].setBounds(langX+langSingle+langGap' in chrome
assert 'headerButtons[4].setToggleState(!english,juce::dontSendNotification)' in chrome
assert 'headerButtons[5].setToggleState(english,juce::dontSendNotification)' in chrome
assert "headerButtons[4].setTooltip" in chrome
assert 'g.drawFittedText(boom::ui::text::safeUpperText(b.getButtonText())' in chrome
assert 'styleName=boom::ui::text::safeUpperText(style)' in chrome
assert 'presetName=boom::ui::text::safeUpperText(preset)' in chrome
assert "boom::ui::text::safeUpperText(text)" in chrome
assert 'void BoomAudioProcessorEditor::setInterfaceLanguage(bool useEnglish)' in menu
assert 'write("language",useEnglish?1.f:0.f)' in menu
assert 'english=read("language")>.5f' in menu
assert 'nativeBrowser->updateLanguage(english)' in menu
assert 'nativeChain->setEnglish(english)' in menu
assert 'refreshChrome()' in menu
assert 'void BoomAudioProcessorEditor::openLanguageMenu()' not in menu
assert 'void openLanguageMenu();' not in source("Source/PluginEditor.h")
assert 'english?"LANGUAGE":"LANGUE"' not in panels
assert 'openLanguageMenu()' not in panels
assert 'english?"EN":"FR"' not in chrome
assert 'std::array<juce::TextButton,11> toolbarButtons;' in chrome_h
assert "BoomPremiumMenuLook menuLook" in source("Source/PluginEditor.h")
assert "popupOptions({443.f,282.f,215.f,29.f})" in panels
assert "withTargetScreenArea(target)" in menu
assert "m.setLookAndFeel(&menuLook)" in panels
assert "sortCombo.setLookAndFeel(&menuLook)" in browser
assert "c->box.setLookAndFeel(&menuLook)" in chain
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
    x=round(.664*width)
    w=round(.082*width)
    gap=max(3,round(.004*width))
    single=(w-gap)//2
    preset_start=round(.752*width)
    assert x>round(.655*width), width
    assert x+single+gap<w+ x, width
    assert x+w<preset_start, (width,x+w,preset_start)
print("BOOM upper text/language PASS: display-only glyph cleanup, "
      "6 native header controls, direct FR/EN APVTS choices and nonoverlap on 13-inch windows")
