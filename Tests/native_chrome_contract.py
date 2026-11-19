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
 "French":"setInterfaceLanguage(false)",
 "English":"setInterfaceLanguage(true)",
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
assert "Master personnalise" in core
assert "BIBLIOTHEQUE" in browser
assert "REGLAGES" in chrome
assert "EGALISEUR" in chain

# Test header and toolbar layout at several plausible physical plugin widths.
for w in (960,1100,1280,1592,1672):
    leftEnd=.262*w
    rightStart=.752*w
    assert leftEnd < .345*w < .655*w < rightStart
    r=min(1.,w/1230.)
    nav=[91,97,109,73]
    compare=[38,38,73,73,58,58,88]
    gap=max(3,round(6*r))
    margin=round(13*r)
    leftEnd=margin+sum(round(z*r)+gap for z in nav)
    compareWidth=sum(round(z*r)+gap for z in compare)
    rightStart=w-margin-compareWidth
    labelWidth=round(78*r)
    assert leftEnd < rightStart-labelWidth-6, (w,"toolbar overlap")

# One lower rendering surface, not four bitmap/vector layers.
# The single full-panel paint eliminates photograph, mask, and live-control
# intersections shown in the customer's 19 Nov screenshot.
from re import findall
mapping=(root/"Source/BOOMUIMap.h").read_text(encoding="utf-8")
assert 'setToolbarArtworkWidth(artWidth())' in core
assert 'toolbarButtons[i].setBounds(' in chrome
assert 'paintLowerButtons(g);' in paint
assert 'paintKnobs(g);' in paint
assert 'paintCleanMeterFaces(g);' in paint
assert 'paintMeter(g);' in paint
assert 'One native surface replaces the ENTIRE lower photographed console' in paint
assert 'g.setGradientFill(panel);g.fillRoundedRectangle(face,5.f);' in paint
assert 'g.fillEllipse(mask)' not in paint
assert 'const auto lowerRow=browser?' not in paint
assert 'const auto row=browser?' not in paint
assert 'const float eraseRadius=' not in paint
assert 'const auto& skin=b?browserSkin:chainSkin;' not in paint
assert 'const char* ids[]={"meterInput","bypass","mono","inputLink","trimMenu"}' in paint
assert 'const auto& z=zones[n]' in paint
assert 'g.drawEllipse(disk.expanded(2.f)' in paint
assert 'juce::Justification::centred:juce::Justification::centredRight' in paint
assert 'const float* centres=b?browserCX:chainCX;' in paint
assert 'chainCX[]={236,505,669,834,1000,1167,1425}' in paint
assert 'browserCY[]={676,797,797,797,797,797,676}' in paint
assert 'chainRadius[]={86,46,46,46,46,46,86}' in paint
assert 'browserRadius[]={80,41,41,41,41,41,80}' in paint
assert 'const float valY=eq?(b?853.f:804.f):(b?799.f:745.f);' in paint
for letter in ('"THE POWER BEHIND YOUR MASTER."',
               '"INPUT DRIVE"', '"PUSH"', '"VU"', '"TRIMS"',
               '"EGALISEUR MUSICAL DE MASTERING - 5 BANDES"'):
    assert letter in paint
assert 'void BoomAudioProcessorEditor::paintCleanMeterFaces(' in paint
assert 'const auto face=inset.reduced(6.f,5.f);' in paint

settings=(root/"Source/PluginEditorPanels.cpp").read_text(encoding="utf-8")
assert 'g.drawText(s.title,s.x,s.y,215,19,juce::Justification::centred)' in settings
assert 'modes[chosen],false,true' in settings

all_knobs=findall(r'\{(\d+),(\d+),(\d+),(\d+),Kind::knob,"(eq60|eq120|eq3k|eq6k|eq10k)"\}',mapping)
assert len(all_knobs)==10
for z,x in zip(all_knobs[:5],(505,669,834,1000,1167)):
    zx,y,width,h,kind=z
    assert abs(int(zx)+int(width)*.5-x)<1.6,(kind,zx,x)
    assert 740<=int(y)+int(h)*.5<=747
for z,x in zip(all_knobs[5:],(497,633,775,914,1052)):
    zx,y,width,h,kind=z
    assert abs(int(zx)+int(width)*.5-x)<2.,(kind,zx,x)
    assert abs(int(y)+int(h)*.5-797.)<1.
assert '{147,836,97,29,Kind::toggle,"inputLink"}' in mapping
assert '{123,884,102,32,Kind::toggle,"inputLink"}' in mapping

# No clipping or overlap for both 13-inch layouts and their original
# design coordinates. These are the regions of labels, scales and readouts.
for name,cy,r,eq_title,value_y,frequency_y,subtitle_y,bottom in [
    ("console",744,46,648,804,830,852,872),
    ("browser",797,41,708,853,878,902,931),
]:
    top_number=cy-(r+19)-8
    side_number_bottom=cy+.704*(r+19)+8
    assert eq_title+18<top_number,(name,"EQ title vs zero graduation")
    assert side_number_bottom<value_y,(name,"EQ graduation vs dB")
    assert value_y+22<frequency_y,(name,"EQ dB vs frequency")
    assert frequency_y+21<=subtitle_y,(name,"frequency vs category")
    assert subtitle_y+18<bottom,(name,"category vs footer")
for name,title_y,button_y in [("console",406,428),("browser",485,507)]:
    assert title_y+19<button_y,(name,"slogan vs VU controls")
for name,secondary_y,link_y in [("console",815,836),("browser",862,884)]:
    assert secondary_y+15<link_y,(name,"input subtitle vs buttons")
print("BOOM lower console contract PASS: one opaque native panel, unique "
      "controls, seven clean knobs, nonoverlapping VU/labels on both layouts")
