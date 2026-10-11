# lib

Dear ImGui 1.92.9b (docking branch) and ImPlot 1.1 WIP for 64-bit Windows, through cimgui and
cimplot, with the Win32 and DirectX 11 backends, a ready window (`native/fels_app.cpp`) and C
functions for the plots (`native/fels_plot.cpp`):

| File | What it is |
| --- | --- |
| `cimgui.dll` | the library; `build.fels` copies it next to the programs that use the package |
| `cimgui.lib` | its import library, to link with |
| `cimgui.h`, `cimplot.h` | the C headers of these versions, for reference |
| `LICENSE-*.txt` | the MIT licenses of Dear ImGui, cimgui, ImPlot and cimplot |

The DLL needs only libraries every Windows 10 and 11 has: Direct3D 11, the D3D compiler 47,
DWM, GDI, User32, the common dialogs and the Universal C Runtime.

## How it was built

From [cimgui](https://github.com/cimgui/cimgui) at `125f397e0982fe9466eb0a162de547be5efec849`
(branch `docking_inter`) with Dear ImGui at `b48d1afbe8ee8b238e2961dc363a949dd7304e23`, and
[cimplot](https://github.com/cimgui/cimplot) at `11f13e6` with ImPlot at `1351ab2`, by
`native/build.sh` with Zig 0.16.0:

```
git clone --branch docking_inter --recurse-submodules https://github.com/cimgui/cimgui.git
git -C cimgui checkout 125f397e0982fe9466eb0a162de547be5efec849
git -C cimgui submodule update --init
git clone --recurse-submodules https://github.com/cimgui/cimplot.git
native/build.sh cimgui cimplot
```
