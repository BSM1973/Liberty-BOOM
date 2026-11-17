#!/usr/bin/env python3
"""BOOM v1.3 UI text regression: accessible labels, centered titles, UTF-8 accents."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
files=[
 "Source/PluginEditor.cpp",
 "Source/PluginEditorPaint.cpp",
 "Source/PluginEditorPanels.cpp",
 "Source/BoomBrowser.cpp",
 "Source/BoomBrowserActions.cpp",
 "Source/BoomChainPanel.cpp",
]
bad="★☆◆✕☷▦▶✓•→—–○"
for file in files:
    content=(root/file).read_text(encoding="utf-8")
    found=sorted(set(content).intersection(bad))
    assert not found,f"{file}: unsupported decorative glyphs: {''.join(found)}"
paint=(root/"Source/PluginEditorPaint.cpp").read_text(encoding="utf-8")
assert 'g.drawFittedText(text,x+6,y,w-12,h,juce::Justification::centred,1);' in paint
for text in ('styleName','presetName'):
    assert paint.count(text)>=3, f"{text}: missing title refresh"
browser=(root/"Source/BoomBrowser.cpp").read_text(encoding="utf-8")
assert 'styleButton(viewList,"LISTE"' in browser
assert 'styleButton(viewGrid,"GRILLE"' in browser
assert 'favorite.setToggleState(starred,juce::dontSendNotification)' in browser
assert '"FAV"' in browser, "Favorites remain visible without a star font"
assert '"★"' not in browser and '"☆"' not in browser
preview=(root/"Source/BoomBrowserActions.cpp").read_text(encoding="utf-8")
assert 'put(item->name,1230,152,286,26,19,true,true)' in preview
assert 'centered?juce::Justification::centred' in preview
chain=(root/"Source/BoomChainPanel.cpp").read_text(encoding="utf-8")
assert '"ÉGALISEUR"' in chain
assert '"DÉSACTIVER"' in chain
assert 'SliderAttachment' in (root/"Source/BoomChainPanel.h").read_text(encoding="utf-8")
cmake=(root/"CMakeLists.txt").read_text(encoding="utf-8")
assert "target_compile_options(BOOM PRIVATE /utf-8)" in cmake
print("BOOM UI typography PASS: centered Style/Preset selectors + preview, "
      "clean decorative labels, intact French accents and favorite state.")
