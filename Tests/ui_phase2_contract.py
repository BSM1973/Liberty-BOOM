#!/usr/bin/env python3
"""BOOM v1.5 phase 2: typography, browser, preview and native-toolbar hygiene."""
from pathlib import Path
import re

root=Path(__file__).resolve().parents[1]
paths=[
 "Source/BoomChrome.cpp","Source/BoomBrowser.cpp",
 "Source/BoomBrowserActions.cpp","Source/BoomChainPanel.cpp",
 "Source/PluginEditor.cpp","Source/PluginEditorPaint.cpp",
 "Source/PluginEditorPanels.cpp"
]
for filename in paths:
    src=(root/filename).read_text(encoding="utf-8")
    assert '#include "BoomUiTheme.h"' in src, f"Missing shared theme: {filename}"
    assert src.isascii(),f"Non-portable source encoding: {filename}"
    assert "Ã" not in src and "\ufffd" not in src
theme=(root/"Source/BoomUiTheme.h").read_text(encoding="utf-8")
assert "inline juce::Font font(float size,bool bold=false)" in theme
assert "inline constexpr float body=13.5f" in theme
cmake=(root/"CMakeLists.txt").read_text(encoding="utf-8")
assert "Source/BoomUiTheme.h" in cmake
chrome=(root/"Source/BoomChrome.cpp").read_text(encoding="utf-8")
assert 'toolbarButtons[i].setTooltip(tips[i]);' in chrome
assert 'g.drawVerticalLine(x,' in chrome
assert 'g.drawFittedText(boom::ui::text::safeUpperText(b.getButtonText()),b.getLocalBounds().reduced(5,1)' in chrome
assert 'juce::Justification::centred' in chrome
assert 'pagesForButtons' in chrome
browser=(root/"Source/BoomBrowser.cpp").read_text(encoding="utf-8")
preview=(root/"Source/BoomBrowserActions.cpp").read_text(encoding="utf-8")
assert "categoriesFR[10]" in browser and "categoriesEN[10]" in browser
assert "categoryDisplay(e.category,english)" in browser
assert "englishFamily.containsIgnoreCase(needle)" in browser
assert "sortCombo.clear(juce::dontSendNotification)" in browser
assert "status.setTooltip(visible)" in browser
assert "load.setTooltip" in browser
assert 'if(grid&&chosen>=0&&chosen/2==row)return' in browser
assert 'put(status,791,306,414,31)' in preview
assert 'put(nameEdit,1230,218,294,26)' in preview
assert 'put(tagsEdit,1230,266,205,26)' in preview
assert 'put(writeTags,1441,266,82,26)' in preview
assert 'write(item->name,1231,148,292,29,19.f,true,true)' in preview
for metric in ('"DRIVE"','"PUSH"','"GLUE"','"PLAFOND"'):
    assert metric in preview, f"Missing current DSP data: {metric}"
assert "boom::makePreset(item->index).params" in preview
assert "boom::makeStyle(item->index)" in preview
assert "write(english?\"CUSTOM TAGS\"" in preview
assert "rule(257)" in preview
# Browser layout in 1550x345 design coordinates. Verify no collision in the
# user editor, native load button, status area or compact list toolbar.
def intersects(a,b):
    ax,ay,aw,ah=a; bx,by,bw,bh=b
    return max(ax,bx)<min(ax+aw,bx+bw) and max(ay,by)<min(ay+ah,by+bh)
rects={
 "status":(791,306,414,31),
 "load":(1217,307,316,30),
 "export":(675,307,102,30),
 "preview":(1221,125,311,175),
 "name":(1230,218,294,26),
 "tags":(1230,266,205,26),
 "write":(1441,266,82,26),
 "userTitle":(1232,202,290,12),
 "userTagTitle":(1232,247,290,13),
}
for a,b in [("status","load"),("status","export"),("name","tags"),
            ("tags","write"),("userTitle","name"),("userTagTitle","tags")]:
    assert not intersects(rects[a],rects[b]), f"Layout collision {a}/{b}"
# No drawn preview content may compete with editable fields in user mode.
assert "return; // Native edit fields occupy y=218..244 and y=266..292." in preview
assert '"EGALISEUR"' in (root/"Source/BoomChainPanel.cpp").read_text()
print("BOOM phase 2 PASS: shared native typography, clean footer, localized "
      "browser, aligned previews, full status and no user-field collisions")
