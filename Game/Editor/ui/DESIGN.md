---
name: Epidemic Editor drafting workspace
colors:
  background: "#111820"
  panel: "#1B2530"
  input: "#111B25"
  text: "#E6EDF5"
  muted: "#9DAFC1"
  border: "#445568"
  accent: "#1665A8"
  selection: "#263F58"
  viewport: "#17222E"
typography:
  body: {fontFamily: "Windows message font", fontSize: "11pt"}
  heading: {fontFamily: "Windows message font", fontSize: "12pt", fontWeight: 600}
  metadata: {fontFamily: "Windows message font", fontSize: "9pt"}
spacing: {gutter: "10dip", margin: "12dip", button: "30dip", propertyRow: "36dip"}
---
# Epidemic Editor visual contract

## Overview
Operate mode: a dark CAD drafting workspace (direction seed 5d698492, assigned 3).
Hierarchy left, dominant Scene canvas center, persistent Inspector right,
activation diagnostics below. Prefab/Graph use the center for current authoring
actions; no visual Graph Canvas or spatial Prefab preview is implied.

## Layout
Minimum outer window 1000x800 at 96 DPI. Side widths are bounded; Scene actions
have a dedicated Inspector tab. Tab changes retain selection and cannot discard
a field draft. Property metadata lives in a scrollable description, not long rows.

## Components
Native TreeView, listbox, edit, combobox, tab and button semantics remain.
Owner-drawn buttons/rows retain their text, disabled and keyboard-focus states.
System high-contrast colors override the palette. No idle animation or polling.
Controls use stable command IDs; presentation owns no authoring/runtime data.

## Direction discipline
Exposure-record challenger: retain value hierarchy and explicit read-only state.
Reference-setting challenger: retain a strict aligned grid and quiet rules.
Orienteering challenger: maintain one selection across canvas and coded Inspector.
Metro challenger: retain clear grouped actions, without oversized/cropped type.
These are clarity disciplines; the drafting workspace is the single visual world.

## Verification
Owned hidden-window captures check layout and owner drawing; native Edit
text is incompletely printed while hidden. Visible desktop, actual monitor DPI,
keyboard focus and native context menus still need manual acceptance. One bounded
capture review and one correction pass; no independent agent review (repo rule).
