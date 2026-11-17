#!/usr/bin/env python3
"""BOOM v0.9: all visible zones bind a real JUCE parameter or an implemented action.

The geometry tests are also compiled in dependency-free C++ and check for
any overlapped target rectangles in the original pixel space.
"""
from pathlib import Path
import re
root=Path(__file__).resolve().parents[1]
ui=(root/"Source/BOOMUIMap.h").read_text(encoding="utf-8")
processor=(root/"Source/PluginProcessor.cpp").read_text(encoding="utf-8")
editor=(root/"Source/PluginEditor.cpp").read_text(encoding="utf-8")
menus=(root/"Source/PluginEditorMenus.cpp").read_text(encoding="utf-8")
view=(root/"Source/PluginEditorPaint.cpp").read_text(encoding="utf-8")
panels=(root/"Source/PluginEditorPanels.cpp").read_text(encoding="utf-8")
cmake=(root/"CMakeLists.txt").read_text(encoding="utf-8")
registered=set(re.findall(r'add(?:Float|Bool|Choice)\(l,\s*"([^"]+)"',processor))
zones=re.findall(r'Kind::(knob|field|toggle|menu|action),"([^"]+)"',ui)
assert zones and len(zones)>=100, f"Expected at least 100 individually mapped zones, got {len(zones)}"
controls=[(kind,key) for kind,key in zones if kind in ("knob","field","toggle")]
missing=sorted({key for kind,key in controls if key not in registered})
assert not missing, "Clickable UI parameters absent from JUCE APVTS: "+", ".join(missing)
for kind,key in zones:
    if kind=="action" and not key.startswith("category"):
        assert f'id=="{key}"' in editor, f"Missing command handler: {key}"
    if kind=="menu":
        assert f'id=="{key}"' in editor, f"Missing menu handler: {key}"
assert "renderDynamicText" not in view
assert "getPixelAt" in view and "paintKnobs" in view and "paintMeter" in view
assert "refreshResults()" in menus and "loadFactoryPreset" in menus
for expected in ("Source/PluginEditor.cpp","Source/PluginEditorPaint.cpp",
                 "Source/PluginEditorPanels.cpp","Source/PluginEditorMenus.cpp",
                 "Source/BOOMUIMap.h","Tests/ui_map_validation.cpp"):
    assert expected in cmake, f"CMake source not registered: {expected}"
for required in ("inputDb","eq60","eq120","eq3k","eq6k","eq10k",
                 "push","colorPushCharacter","outputBypass","limiterAutoRelease",
                 "outPeakGuard","outputLevelComp"):
    assert required in registered, f"Required DSP control missing: {required}"
print(f"BOOM real UI contract: PASS, {len(zones)} mapped zones, "
      f"{len(controls)} parameter-using targets, "
      f"{len(registered)} JUCE parameters, no unknown or fake control.")
