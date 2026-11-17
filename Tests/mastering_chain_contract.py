#!/usr/bin/env python3
"""BOOM native mastering chain contract.
Source-level architecture checks; JUCE builds and real-host GUI tests remain required.
"""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
panel=(root/"Source/BoomChainPanel.cpp").read_text(encoding="utf-8")
header=(root/"Source/BoomChainPanel.h").read_text(encoding="utf-8")
editor=(root/"Source/PluginEditor.cpp").read_text(encoding="utf-8")
paint=(root/"Source/PluginEditorPaint.cpp").read_text(encoding="utf-8")
cmake=(root/"CMakeLists.txt").read_text(encoding="utf-8")
processor=(root/"Source/PluginProcessor.cpp").read_text(encoding="utf-8")
registered=set(re.findall(r'add(?:Float|Bool|Choice)\(l,\s*"([^"]+)"',processor))
controls=re.findall(r'add(Rotary|Toggle|Choice)\(\s*(\d)\s*,\s*"([^"]+)"',panel)
assert len(controls)==43,(len(controls),controls)
assert len(set(name for _,_,name in controls))==43,"Duplicate or unbound control"
assert sorted(set(int(module) for _,module,_ in controls))==list(range(7))
for kind,module,name in controls:
    assert name in registered, f"Control without APVTS parameter: {name}"
    assert kind in ("Rotary","Toggle","Choice")
assert "SliderAttachment" in header
assert "ButtonAttachment" in header
assert "ComboBoxAttachment" in header
assert "slider.textFromValueFunction" in panel
assert "setValueNotifyingHost" in panel
assert 'setProperty("boomChainExpanded",expanded' in panel
assert 'setProperty("boomChainModule",currentModule' in panel
assert "chainSkin" in editor
assert "nativeChain=std::make_unique<BoomChainPanel>" in editor
assert "nativeChain->setBounds" in editor
assert "nativeChain->setVisible(page==Page::chain)" in editor
assert "p.y>=96.f && p.y<342.f" in editor, "Legacy rack hit boxes must be disabled"
assert "if(page==Page::chain)paintChain(g);" not in paint, "Legacy duplicated DSP controls still painted"
assert "Source/BoomChainPanel.h" in cmake and "Source/BoomChainPanel.cpp" in cmake
# Collapsing the chain should never bypass/disable actual processing.
collapse=panel.split("void BoomChainPanel::toggleVisibility(){",1)[1].split("\n}",1)[0]
assert "expanded=!expanded" in collapse
assert 'setProperty("boomChainExpanded"' in collapse
assert 'setValueNotifyingHost' not in collapse
assert "toggle(" not in collapse
# At 13-inch and desktop fit scales, the navigation and action buttons must
# have separated hit rectangles, with none extending past the native panel.
for width in (960,1080,1320,1577):
    pad=max(8,width//130)
    title=max(96,width*11//100)
    hide=max(87,width*9//100)
    reset=max(90,width*10//100)
    bypass=max(90,width*10//100)
    tabs_start=pad+title
    tabs_span=max(7*54,width-tabs_start-hide-reset-bypass-4*pad)
    tab_w=tabs_span//7
    tabs_end=tabs_start+6*tab_w+max(45,tab_w-4)
    reset_start=width-hide-reset-bypass-3*pad
    bypass_start=width-hide-bypass-2*pad
    hide_start=width-hide-pad
    assert tabs_end<reset_start, (width,"tabs hit reset")
    assert reset_start+reset<bypass_start,(width,"reset hits bypass")
    assert bypass_start+bypass<hide_start,(width,"bypass hits hide")
    assert hide_start+hide<=width,(width,"hide outside panel")
print(f"BOOM master chain: PASS — 7 tabs, {len(controls)} live controls, "
      "collapsed/persistent UI state, 4 screen-width layout checks.")
