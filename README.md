# imgui

[Fels](https://github.com/fels-lang/fels) bindings to [Dear ImGui](https://github.com/ocornut/imgui),
the docking branch, through [cimgui](https://github.com/cimgui/cimgui).

**State:** created for Strata (milestone M3). The code is still the C binding template; the bindings come with M3.

## Planned

- cimgui built as a DLL from the docking branch, with the Win32 and DirectX 11 backends, in
  `lib/`, provided to the users of the package by `build.fels`.
- The bindings in `src/imgui.fels`, written by `fels bindgen cimgui.h --lib cimgui` and checked
  by hand where ImGui passes structs by value (`ImVec2`, `ImVec4`).
- A small Fels layer: create the window and the device, run the frame loop, docking space,
  load fonts (with tabular digits), and themes.
- Tables (sorting, hiding and reordering columns, the list clipper for many rows), menus and
  popups, enough for a command palette.

## Use it

```
fels add imgui github.com/<you>/imgui
```

```fels
#import imgui;
```

## Work on it

```
fels test        run the tests
fels build run   build and run examples/demo.fels
```
