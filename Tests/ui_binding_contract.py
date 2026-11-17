#!/usr/bin/env python3
"""Fail CI if a visible BOOM mastering control has no APVTS binding."""
from pathlib import Path
import re
root = Path(__file__).resolve().parents[1]
processor = (root / "Source/PluginProcessor.cpp").read_text(encoding="utf-8")
editor = (root / "Source/PluginEditor.cpp").read_text(encoding="utf-8")
dsp = (root / "Source/BoomDSP.h").read_text(encoding="utf-8")
registered = set(re.findall(r'add(?:Float|Bool|Choice)\(l,\s*"([^"]+)"', processor))
visible_numeric = (
    "inputDb", "eq60", "eq120", "eq3k", "eq6k", "eq10k", "push",
    "colorMix", "colorDrive", "deFreq", "deTh", "deAmt", "deRel",
    "compTh", "compRatio", "compAttack", "compRelease", "compMakeup",
    "compMix", "limitTh", "ceiling", "limitRelease", "outDb"
)
visible_switches = (
    "colorBypass", "eqBypass", "deEssBypass", "compBypass",
    "limiterBypass", "compStereoLink", "limiterStereoLink",
    "limiterAutoRelease", "outPeakGuard", "outputLevelComp",
    "inputLink", "mono", "bypass", "meterInput", "language"
)
for name in visible_numeric + visible_switches:
    assert name in registered, f"Missing APVTS parameter: {name}"
    assert f'"{name}"' in editor, f"Missing GUI binding for {name}"
for marker in (
    "void BoomAudioProcessorEditor::renderChainControls",
    "void BoomAudioProcessorEditor::commitNumeric()",
    "void BoomAudioProcessorEditor::openFieldEditor(",
    "void BoomAudioProcessorEditor::showTrimsMenu()",
    "void BoomAudioProcessorEditor::cyclePreset("
):
    assert marker in editor, f"Missing interactive GUI hook: {marker}"
assert 'notice="Auto Release : contrôlé' not in editor
for ident in ("limiterAutoRelease", "limiterStereoLink", "outputPeakGuard", "outputLevelComp"):
    assert ident in dsp, f"Missing DSP path for {ident}"
print(f"BOOM UI contract PASS: {len(visible_numeric)} numeric controls, "
      f"{len(visible_switches)} switches, and preset/navigation hooks.")
