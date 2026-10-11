#!/bin/sh
# Build lib/cimgui.dll and lib/cimgui.lib from Dear ImGui (docking branch), cimgui, ImPlot and
# cimplot, the Win32 and DirectX 11 backends, native/fels_app.cpp and native/fels_plot.cpp, with
# Zig's C++ compiler:
#
#   git clone --depth 1 --branch docking_inter --recurse-submodules --shallow-submodules \
#       https://github.com/cimgui/cimgui.git
#   git clone --recurse-submodules https://github.com/cimgui/cimplot.git
#   native/build.sh path/to/cimgui path/to/cimplot
#
# lib/README.md says which commits the DLL in lib/ was built from.
set -e
C=${1:?usage: native/build.sh path/to/cimgui path/to/cimplot}
P=${2:?usage: native/build.sh path/to/cimgui path/to/cimplot}
HERE=$(cd "$(dirname "$0")" && pwd)
OUT="$HERE/../lib"
zig c++ -target x86_64-windows-gnu -O2 -shared -std=c++17 -fno-exceptions -fno-rtti \
    -I"$C" -I"$C/imgui" -I"$C/imgui/backends" -I"$P" -I"$P/implot" \
    "$C/cimgui.cpp" \
    "$C/imgui/imgui.cpp" "$C/imgui/imgui_draw.cpp" "$C/imgui/imgui_tables.cpp" \
    "$C/imgui/imgui_widgets.cpp" "$C/imgui/imgui_demo.cpp" \
    "$C/imgui/backends/imgui_impl_win32.cpp" "$C/imgui/backends/imgui_impl_dx11.cpp" \
    "$P/implot/implot.cpp" "$P/implot/implot_items.cpp" "$P/implot/implot_demo.cpp" \
    "$P/cimplot.cpp" \
    "$HERE/fels_app.cpp" "$HERE/fels_plot.cpp" \
    -ld3d11 -ld3dcompiler_47 -ldxgi -lgdi32 -ldwmapi -luser32 -limm32 -lcomdlg32 \
    -s -o "$OUT/cimgui.dll"
rm -f "$OUT/cimgui.pdb"
cp "$C/cimgui.h" "$OUT/cimgui.h"
cp "$P/cimplot.h" "$OUT/cimplot.h"
