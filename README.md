# imgui

[Fels](https://github.com/fels-lang/fels) bindings to [Dear ImGui](https://github.com/ocornut/imgui)
1.92.9b, the docking branch, through [cimgui](https://github.com/cimgui/cimgui), with a ready
window on Direct3D 11 and the library for 64-bit Windows included.

```fels
#import imgui;

main :: fn() -> i32 {
    if !imgui::open("Notes", 1280, 800, imgui::DOCKING | imgui::KEYBOARD, "notes.ini") { return 1; }
    while imgui::begin_frame() {
        dock := imgui::dock_space();
        if imgui::begin("Hello") {
            imgui::text("Hello from Fels");
            if imgui::button("Click") { imgui::text("clicked"); }
        }
        imgui::end();
        imgui::end_frame(0.10, 0.11, 0.12);
    }
    imgui::close();
    return 0;
}
```

## Use it

```
fels add imgui github.com/fels-lang/imgui
```

The package's `build.fels` gives your programs the library folder and copies `cimgui.dll` next
to them. A program with a window has no console; in `build.fels`: `app.windowed();`.

## What is there

- **The window:** `open`, `begin_frame`, `end_frame`, `close`, `quit`, `dpi_scale`,
  `load_font`. The layout of docked panels is kept in the ini file `open` names, or by the
  program itself with `layout_text`, `layout_changed` and `load_layout`.
- **The style:** `set_color` (the `COL_*` indexes), `set_rounding`, `set_spacing`,
  `set_borders`, `set_font_scale`, `set_title_bar` (a dark or light title bar in the color of
  the panels).
- **Panels and layout:** `begin`, `begin_with`, `end`, `dock_space`, `dock_window`,
  `child_begin`, `same_line`, `separator`, `avail`, `text_size`, ...
- **Layouts:** `layout_begin`, `layout_split` and `layout_end` lay out the dock space anew,
  for a first start or a preset; `dock_window` puts a panel into a part.
- **Widgets:** `text`, `text_colored`, `text_right` (numbers in a column), `button`,
  `checkbox`, `selectable`, `input_text`, `combo_begin`, `tooltip`, `pressed` (keys with
  `MOD_CTRL` and the like).
- **Menus and popups:** `main_menu_bar_begin`, `menu_begin`, `menu_item`, `menu_toggle`,
  `popup_open`, `popup_modal_begin`.
- **Tables:** `table_begin` with `TABLE_*` flags (resizable, reorderable, hideable and sortable
  columns, scrolling), `table_column`, `table_freeze`, `table_headers`, `table_row`,
  `table_next_column`, `table_sort_specs`.

Texts given as labels are copied into short C strings (255 bytes); `text` takes any length.
Everything else of Dear ImGui is in `cimgui.dll` too: bind more of `lib/cimgui.h` with
`#extern("cimgui", "ig...")` as `src/imgui.fels` does.

## Work on it

```
fels build run   build and run examples/demo.fels
```

`native/` holds the C++ window code and the build script; `lib/README.md` says how the DLL
was built.
