# lib

Dear ImGui 1.92.9b (docking branch) for 64-bit Windows, through cimgui, with the Win32 and
DirectX 11 backends and a ready window (`native/fels_app.cpp`):

| File | What it is |
| --- | --- |
| `cimgui.dll` | the library; `build.fels` copies it next to the programs that use the package |
| `cimgui.lib` | its import library, to link with |
| `cimgui.h` | the C header of this version, for reference |
| `LICENSE-imgui.txt`, `LICENSE-cimgui.txt` | the MIT licenses of Dear ImGui and cimgui |

The DLL needs only libraries every Windows 10 and 11 has: Direct3D 11, the D3D compiler 47,
DWM, GDI, User32, the common dialogs and the Universal C Runtime.

## How it was built

From [cimgui](https://github.com/cimgui/cimgui) at `125f397e0982fe9466eb0a162de547be5efec849`
(branch `docking_inter`) with Dear ImGui at `b48d1afbe8ee8b238e2961dc363a949dd7304e23`, by
`native/build.sh` with Zig 0.16.0:

```
git clone --depth 1 --branch docking_inter --recurse-submodules --shallow-submodules https://github.com/cimgui/cimgui.git
native/build.sh cimgui
```
