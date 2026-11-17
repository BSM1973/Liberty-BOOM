#!/usr/bin/env python3
"""BOOM v1.6 phase 3 — layout + interaction invariants, including 13" screens.

Source contracts and geometry checks complement native AU/VST3 compilation.
They do not replace actual DAW visual and keyboard verification.
"""
from pathlib import Path
from math import floor

root=Path(__file__).resolve().parents[1]
read=lambda name:(root/name).read_text(encoding="utf-8")
theme=read("Source/BoomUiTheme.h")
chrome=read("Source/BoomChrome.cpp")
browser_h=read("Source/BoomBrowser.h")
browser=read("Source/BoomBrowser.cpp")
actions=read("Source/BoomBrowserActions.cpp")
chain=read("Source/BoomChainPanel.cpp")
editor_h=read("Source/PluginEditor.h")
editor=read("Source/PluginEditor.cpp")
paint=read("Source/PluginEditorPaint.cpp")
workflow=read(".github/workflows/build.yml")

assert "inline void focusRing(" in theme
for src in (chrome,browser,chain):
    assert "boom::ui::theme::focusRing(" in src
    assert "hasKeyboardFocus(true)" in src
    assert "setWantsKeyboardFocus(true)" in src
assert "isEnabled()" in chrome and "isEnabled()" in browser and "isEnabled()" in chain
assert "hovered?gold.withAlpha" in browser
assert "hover||pressed" in chain
assert "hoveredIndex" in browser_h and "hoveredCategory" in browser_h
assert "void mouseMove(" in browser_h and "void mouseExit(" in browser_h
assert "void BoomBrowser::mouseMove(" in actions
assert "void BoomBrowser::mouseExit(" in actions
assert "items.addMouseListener(this,true)" in browser
assert "categories.addMouseListener(this,true)" in browser
assert "items.removeMouseListener(this)" in browser
assert "categories.removeMouseListener(this)" in browser
hover=actions.split("void BoomBrowser::mouseMove(",1)[1].split(
      "\nvoid BoomBrowser::mouseExit(",1)[0]
assert "loadSelection(" not in hover
assert "setValueNotifyingHost" not in hover
assert "items.getRowContainingPosition" in hover
assert "categories.getRowContainingPosition" in hover

assert "hoveredKnob" in editor_h and "focusedKnob" in editor_h
assert "void BoomAudioProcessorEditor::mouseMove(" in editor
assert "void BoomAudioProcessorEditor::mouseExit(" in editor
assert "focusedKnob=i" in editor and "grabKeyboardFocus()" in editor
assert "k.getModifiers().isShiftDown()" in editor
assert "hoveredKnob==i" in paint and "focusedKnob==i" in paint
assert "g.drawEllipse(disk.expanded(2.f)" in paint
assert "processor.state().getParameter(id)" in editor
assert "Source/BoomUiTheme.h" in read("CMakeLists.txt")

# User preset fields have real gutters; no label or button collisions.
rectangles={
 "name_caption":(1232,202,290,12),
 "name_field":(1230,218,294,26),
 "tags_caption":(1232,247,290,13),
 "tags_field":(1230,266,205,26),
 "tags_save":(1441,266,82,26),
 "status":(791,306,414,31),
 "load":(1217,307,316,30),
 "export":(675,307,102,30),
 "sidebar":(15,125,214,175),
 "items":(240,125,968,175),
 "preview":(1221,125,311,175),
}
def roundi(v):return floor(v+0.5)
def intersects(a,b):
    ax,ay,aw,ah=a;bx,by,bw,bh=b
    return max(ax,bx)<min(ax+aw,bx+bw) and max(ay,by)<min(ay+ah,by+bh)
for left,right in [
 ("name_caption","name_field"),("name_field","tags_caption"),
 ("tags_caption","tags_field"),("tags_field","tags_save"),
 ("status","load"),("status","export"),("sidebar","items"),("items","preview")]:
    assert not intersects(rectangles[left],rectangles[right]),(left,right)

for work_w,work_h in [(1280,800),(1440,900),(1366,768),(1280,720)]:
    # Mirror the editor's original-size fit, accounting for the DAW title bar.
    ratio=min(1.0,(work_w-44)/1672,(work_h-72)/941)
    assert ratio>0.65
    editor_w,editor_h=roundi(1672*ratio),roundi(941*ratio)
    assert editor_w<=work_w-44+1 and editor_h<=work_h-72+1
    # Browser is a different reference-size view, fitted independently.
    br=min(1.0,(work_w-44)/1592,(work_h-72)/988)
    assert roundi(1592*br)<=work_w-44+1
    assert roundi(988*br)<=work_h-72+1
    panel_w,panel_h=roundi(1557*br),roundi(337*br)
    sx,sy=panel_w/1550,panel_h/345
    assert roundi(30*sy)>=18,(work_w,work_h,"browser buttons too short")
    def scaled(rect):
        x,y,w,h=rect
        return (roundi(x*sx),roundi(y*sy),roundi(w*sx),roundi(h*sy))
    for left,right in [
     ("name_caption","name_field"),("name_field","tags_caption"),
     ("tags_caption","tags_field"),("tags_field","tags_save"),
     ("status","load"),("sidebar","items"),("items","preview")]:
        assert not intersects(scaled(rectangles[left]),scaled(rectangles[right])),(
            work_w,work_h,left,right)
    # The toolbar is segmented into a left navigation group, a comparison
    # label, and actual A/B buttons. Check nonintersecting groups.
    chrome_w=editor_w
    factor=min(1.,chrome_w/1230)
    gap=max(3,roundi(6*factor))
    margin=roundi(13*factor)
    nav=[91,97,109,73,56]
    compare=[38,38,73,73,58,58,88]
    left_end=margin+sum(roundi(v*factor)+gap for v in nav)
    compare_start=chrome_w-margin-sum(roundi(v*factor)+gap for v in compare)
    label_start=compare_start-roundi(78*factor)-6
    assert left_end+8<label_start,(work_w,work_h,"toolbar overlap")
    # Header buttons stay separated and within the native header component.
    header_right=roundi(.752*chrome_w)
    right_width=roundi(.216*chrome_w)
    arrow=max(23,min(34,roundi(.025*chrome_w)))
    hg=max(4,roundi(.004*chrome_w))
    assert header_right+right_width<=chrome_w
    assert header_right+arrow+hg < header_right+right_width-arrow
    # Mastering panel header must not put 7 tabs underneath reset/bypass/hide.
    chain_w=roundi(1577*ratio)
    pad=max(8,chain_w//130)
    title=max(96,chain_w*11//100)
    hide=max(87,chain_w*9//100)
    reset=max(90,chain_w*10//100)
    bypass=max(90,chain_w*10//100)
    tabs_x=pad+title
    reset_x=chain_w-hide-reset-bypass-3*pad
    tabs_w=max(7*27,reset_x-tabs_x-pad)
    tab_w=tabs_w//7
    tabs_end=tabs_x+6*tab_w+max(26,tab_w-4)
    bypass_x=chain_w-hide-bypass-2*pad
    hide_x=chain_w-hide-pad
    assert tabs_end<reset_x and reset_x+reset<bypass_x
    assert bypass_x+bypass<hide_x and hide_x+hide<=chain_w
for snippet in (
 "const int resetX=w-hideW-resetW-bypassW-3*pad",
 "const int tabsW=juce::jmax(7*27,resetX-tabsX-pad)",
 "resetButton.setBounds(resetX",
):
    assert snippet in chain

assert "python3 Tests/ui_phase3_contract.py" in workflow
print("BOOM phase 3 PASS: native focus/hover, state separation, keyboard knobs, "
      "13-inch constraints and non-overlapping labels/actions")
