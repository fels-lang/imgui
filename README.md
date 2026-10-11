# imgui

[Fels](https://github.com/fels-lang/fels) bindings to [Dear ImGui](https://github.com/ocornut/imgui)
1.92.9b, the docking branch, through [cimgui](https://github.com/cimgui/cimgui), with a ready
window on Direct3D 11, plots by [ImPlot](https://github.com/epezent/implot) (`imgui.plot`), and
the library for 64-bit Windows included.

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
  `load_font`, `open_file_dialog` and `save_file_dialog` (the file dialogs of Windows). The layout of docked panels is kept in the ini file `open` names, or by the
  program itself with `layout_text`, `layout_changed` and `load_layout`.
- **The style:** `set_color` (the `COL_*` indexes), `set_rounding`, `set_spacing`,
  `set_borders`, `set_font_scale`, `set_title_bar` (a dark or light title bar in the color of
  the panels); `push_color` and `pop_color` for a few items (a button in the accent),
  `push_font_size` and `pop_font` for larger text.
- **Panels and layout:** `begin`, `begin_with`, `end`, `dock_space`, `dock_window`,
  `child_begin`, `same_line`, `separator`, `spacing`, `avail`, `text_size`, `cursor_x` and
  `cursor_y`, `align_text_to_frame` (a label on the line of the field beside it), `button_width`
  and `item_spacing_x` (to put buttons at the right), ...
- **Layouts:** `layout_begin`, `layout_split` and `layout_end` lay out the dock space anew,
  for a first start or a preset; `dock_window` puts a panel into a part.
- **Widgets:** `text`, `text_colored`, `text_right` (numbers in a column), `button`,
  `checkbox`, `selectable`, `input_text`, `input_hint`, `combo_begin`, `plot_lines`, `tooltip`, `pressed` (keys with
  `MOD_CTRL` and the like), `is_item_hovered`, `is_item_double_clicked`.
- **Drawing:** `cursor_screen_pos`, `draw_rect` and `draw_line` draw into the current window,
  for bars and markers of your own.
- **Tabs:** `tab_bar_begin`, `tab_item_begin`, `tab_item_end`, `tab_bar_end`; the selected tab
  has a line in `COL_TAB_SELECTED_OVERLINE` over it.
- **Menus and popups:** `main_menu_bar_begin`, `menu_begin`, `menu_item`, `menu_toggle`,
  `popup_open`, `popup_modal_begin`, `context_menu_begin` (a right click on the last item).
- **Tables:** `table_begin` with `TABLE_*` flags (resizable, reorderable, hideable and sortable
  columns, scrolling), `table_column`, `table_column_right` (its header at the right, over
  numbers), `table_freeze`, `table_headers`, `table_row`, `table_row_color` (a row of sums),
  `table_next_column`, `table_sort_specs`.

## Plots

`#import imgui.plot;` brings ImPlot: lines, areas, stairs, bars (grouped and stacked), scatter,
pie charts, heatmaps with a colour scale, lines across a plot, text and tags, on axes that zoom,
pan and fit, with a legend to hide items, subplots, and date axes (`axis_dates` writes them the
German way: "15.01.", "Okt 25", "2025"; `axis_time` the way of ImPlot). `axis_german` writes the
numbers of an axis the German way at round steps ("40.000 €", "-0,25 %"), and
`colormap_scale_german` and `pie_german` do the same for a heatmap's scale and a pie's slices.

```fels
if plot::begin("##wealth", -1.0, 280.0, plot::NO_TITLE) {
    plot::axes("", "", plot::AXIS_AUTO_FIT, plot::AXIS_AUTO_FIT);
    plot::axis_dates(plot::X1);
    plot::shaded("##area", days, values, 0.0, teal, 0.25, plot::ITEM_NO_LEGEND);
    plot::line("Wert", days, values, teal, 2.0, 0);
    plot::end();
}
```

Each item takes its colour (`auto_color()` for the next one of the colormap) and a weight in
pixels; `style` takes the plot colours from the ImGui style and sets the colours of the items,
`add_colormap` and `push_colormap` give a heatmap its scale. `examples/plots.fels` shows them
(`fels build run-plots`), and `examples/waves.fels` two moving waves in about 40 lines
(`fels build run-waves`).

## Notes

Texts given as labels are copied into short C strings (255 bytes); `text` takes any length.
Everything else of Dear ImGui is in `cimgui.dll` too: bind more of `lib/cimgui.h` with
`#extern("cimgui", "ig...")` as `src/imgui.fels` does.

## Work on it

```
fels build run         build and run examples/demo.fels
fels build run-plots   build and run examples/plots.fels
fels build run-waves   build and run examples/waves.fels
```

`native/` holds the C++ code of the window and the plots and the build script; `lib/README.md`
says how the DLL was built.
