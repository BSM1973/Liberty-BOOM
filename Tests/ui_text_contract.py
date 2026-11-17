#!/usr/bin/env python3
"""BOOM v1.3 UI text regression: accessible labels, centered titles, UTF-8 accents."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
files=[
 "Source/PluginEditor.cpp",
 "Source/PluginEditorPaint.cpp",
 "Source/PluginEditorPanels.cpp",
 "Source/PluginEditorMenus.cpp",
 "Source/BoomBrowser.cpp",
 "Source/BoomBrowserActions.cpp",
 "Source/BoomChainPanel.cpp",
]
bad="★☆◆✕☷▦▶✓•→—–○…"
for file in files:
    content=(root/file).read_text(encoding="utf-8")
    found=sorted(set(content).intersection(bad))
    assert not found,f"{file}: unsupported decorative glyphs: {''.join(found)}"
paint=(root/"Source/PluginEditorPaint.cpp").read_text(encoding="utf-8")
chrome=(root/"Source/BoomChrome.cpp").read_text(encoding="utf-8")
assert "void BoomAudioProcessorEditor::paintHead(" not in paint, "Legacy overpaint must stay deleted"
assert 'g.drawFittedText(b.getButtonText(),b.getLocalBounds().reduced(5,1),' in chrome
assert "juce::Justification::centred" in chrome, "Native header selectors must be centered"
assert "setNames(" in chrome and "setButtonText(styleName)" in chrome
assert "setButtonText(presetName)" in chrome
assert r"R\u00C9GLAGES" in chrome and r"CHA\u00CENE" in chrome
browser=(root/"Source/BoomBrowser.cpp").read_text(encoding="utf-8")
assert 'styleButton(viewList,"LISTE"' in browser
assert 'styleButton(viewGrid,"GRILLE"' in browser
assert 'favorite.setToggleState(starred,juce::dontSendNotification)' in browser
assert '"FAV"' in browser, "Favorites remain visible without a star font"
assert '"★"' not in browser and '"☆"' not in browser
preview=(root/"Source/BoomBrowserActions.cpp").read_text(encoding="utf-8")
assert 'write(item->name,1231,148,292,29,19.f,true,true)' in preview
assert 'centered?juce::Justification::centred' in preview
chain=(root/"Source/BoomChainPanel.cpp").read_text(encoding="utf-8")
assert r'"\u00C9GALISEUR"' in chain
assert r'"D\u00C9SACTIVER"' in chain
assert 'SliderAttachment' in (root/"Source/BoomChainPanel.h").read_text(encoding="utf-8")
cmake=(root/"CMakeLists.txt").read_text(encoding="utf-8")
assert "target_compile_options(BOOM PRIVATE /utf-8)" in cmake
print("BOOM native typography PASS: no legacy header overpainting, native centered selectors, "
      "clean decorative labels, intact French accents and favorite state.")
