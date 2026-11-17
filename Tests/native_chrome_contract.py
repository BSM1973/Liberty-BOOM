#!/usr/bin/env python3
"""BOOM phase 1: native chrome; no jpeg controls, mojibake, or invisible fake actions.

These assertions protect architecture and text bytes. Native builds validate C++;
final optical verification is still done in a DAW.
"""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
core=(root/"Source/PluginEditor.cpp").read_text(encoding="utf-8")
head=(root/"Source/PluginEditor.h").read_text(encoding="utf-8")
paint=(root/"Source/PluginEditorPaint.cpp").read_text(encoding="utf-8")
chrome=(root/"Source/BoomChrome.cpp").read_text(encoding="utf-8")
chrome_h=(root/"Source/BoomChrome.h").read_text(encoding="utf-8")
browser=(root/"Source/BoomBrowserActions.cpp").read_text(encoding="utf-8")
chain=(root/"Source/BoomChainPanel.cpp").read_text(encoding="utf-8")
menu=(root/"Source/PluginEditorMenus.cpp").read_text(encoding="utf-8")
cmake=(root/"CMakeLists.txt").read_text(encoding="utf-8")
for filename in ("Source/BoomChrome.h","Source/BoomChrome.cpp"):
    assert filename in cmake, f"{filename} not built with JUCE"
for identifier in ("nativeHeader","nativeToolbar"):
    assert identifier in head and identifier in core
assert 'std::make_unique<BoomChrome>(BoomChrome::Area::Header' in core
assert 'std::make_unique<BoomChrome>(BoomChrome::Area::Toolbar' in core
assert 'nativeHeader->toFront(false)' in core
assert 'nativeToolbar->toFront(false)' in core
assert 'p.y<481.f : p.y<396.f' in core, "Old JPG hit boxes still consume chrome clicks"
for key in ("headerBottom","toolbarTop","toolbarBottom","g.reduceClipRegion(part)"):
    assert key in paint, f"Source image not masked around native chrome: {key}"
assert "void BoomAudioProcessorEditor::paintHead(" not in paint
assert "paintHead(g)" not in paint
assert "rotation(angle,r,r)" not in paint, "Photographic knobs must not be rotated"
assert "void BoomAudioProcessorEditor::paintKnobs(" in paint
assert 'juce::Justification::centred' in chrome
assert 'headerButtons[0].setButtonText(styleName)' in chrome
assert 'headerButtons[2].setButtonText(presetName)' in chrome
assert "juce::TextButton" in chrome_h
assert "pagesForButtons" in chrome and "0,2,3,4" in chrome, (
    "Native navigation buttons must map to Chain/Meters/Settings/Help, "
    "skipping the Browser page enum")
assert "onClick=" in chrome

required={
 "Style":"openStyleMenu()", "Preset":"openPresetMenu()",
 "Previous":"cyclePreset(-1)","Next":"cyclePreset(1)",
 "Chain":"showPage(Page::chain)","Meters":"showPage(Page::meters)",
 "Settings":"showPage(Page::settings)","Help":"showPage(Page::help)",
 "Language":"openLanguageMenu()",
 "RecallA":"processor.recallComparisonState(0)",
 "RecallB":"processor.recallComparisonState(1)",
 "StoreA":"processor.storeComparisonState(0)",
 "StoreB":"processor.storeComparisonState(1)",
 "CopyAtoB":"processor.copyComparisonState(0,1)",
 "CopyBtoA":"processor.copyComparisonState(1,0)",
 "Level":'toggle("outputLevelComp")'
}
for name,call in required.items():
    assert f'case Cmd::{name}:' in core, f"Missing action: {name}"
    assert call in core, f"Missing actual wiring: {name}: {call}"
for name in required:
    assert f'Command::{name}' in chrome, f"Unwired native JUCE button: {name}"

# French accents are explicit C++ Unicode codepoint escapes, not platform
# dependent input-codepage characters or already corrupted mojibake.
display_sources=[
 "Source/BoomChrome.cpp","Source/PluginEditor.cpp",
 "Source/BoomBrowser.cpp","Source/BoomBrowserActions.cpp",
 "Source/BoomChainPanel.cpp","Source/PluginEditorPanels.cpp",
 "Source/PluginEditorMenus.cpp"
]
for filename in display_sources:
    src=(root/filename).read_text(encoding="utf-8")
    assert src.isascii(),f"{filename}: non-ASCII source literals should use C++ Unicode escapes"
    assert "Ã" not in src and "\ufffd" not in src
assert r"Master personnalis\u00E9" in core
assert r"BIBLIOTH\u00C8QUE" in browser
assert r"R\u00C9GLAGES" in chrome
assert r"\u00C9GALISEUR" in chain

# Test header and toolbar layout at several plausible physical plugin widths.
for w in (960,1100,1280,1592,1672):
    leftEnd=.262*w
    rightStart=.752*w
    assert leftEnd < .345*w < .655*w < rightStart
    r=min(1.,w/1230.)
    nav=[91,97,109,73,56]
    compare=[38,38,73,73,58,58,88]
    gap=max(3,round(6*r))
    margin=round(13*r)
    leftEnd=margin+sum(round(z*r)+gap for z in nav)
    compareWidth=sum(round(z*r)+gap for z in compare)
    rightStart=w-margin-compareWidth
    labelWidth=round(78*r)
    assert leftEnd < rightStart-labelWidth-6, (w,"toolbar overlap")

print("BOOM chrome contract PASS: 16 native actions, real JPEG exclusions, "
      "centred Style/Preset, ASCII-source UTF-8 Unicode, nonoverlapping layouts")
