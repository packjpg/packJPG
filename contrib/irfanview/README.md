# packJPG IrfanView & Susie Plug-in

Standalone Windows image plug-in (`x64` and `x86`) enabling **IrfanView** (as well as Susie-compatible viewers such as **XnView** and **Total Commander**) to view, thumbnail, and save `.pjg` (`packJPG`) files using `packJPGlib` and the built-in Windows Imaging Component (`WIC`).

## Installation in IrfanView (Works Immediately via Susie `00IN`)

1. Copy **`ifpjg.sph`** (for 64-bit IrfanView) or **`ifpjg.spi`** (for 32-bit IrfanView) into your IrfanView `Plugins\` folder (e.g. `C:\Program Files\IrfanView\Plugins\`).
2. Open IrfanView → **Options → Properties/Settings → PlugIns** and ensure Susie plug-ins are enabled (or add `.pjg` under **Extensions → Load custom file types**).
3. Open any `.pjg` file directly in IrfanView or browse `.pjg` folders in IrfanView Thumbnails.

## Native IrfanView Format Plug-in (`PackJPGx64.dll` / `PackJPGx86.dll`)

`PackJPGx64.dll` and `PackJPGx86.dll` also export native IrfanView format entrypoints (`__cdecl`, ANSI & UTF-16) for full read **and** save integration:

```c
int    __cdecl GetPlugInInfo(char* version, char* formats);
HANDLE __cdecl ReadPJG_W(LPCWSTR filename, UINT flags, LPWSTR errbuf);
HANDLE __cdecl ReadPJG(LPCSTR filename, UINT flags, LPSTR errbuf);
int    __cdecl SavePJG_W(LPCWSTR filename, HANDLE hDib, int quality, LPWSTR errbuf);
int    __cdecl SavePJG(LPCSTR filename, HANDLE hDib, int quality, LPSTR errbuf);
```

## Building & Testing (MSYS2 MinGW-w64)

```bash
make -C contrib/irfanview clean all test
```
