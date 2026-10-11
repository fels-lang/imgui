# lib

Put the C library here: `imgui.lib` to link with, and `imgui.dll` when it is a DLL.
Then bind its functions in `src/imgui.fels`:

```fels
#extern("imgui", "imgui_init")
pub init :: fn(flags: u32) -> i32;
```

and in `build.fels` uncomment the lines for the DLL. `fels bindgen imgui.h --lib imgui`
writes the bindings of a whole header for you.
